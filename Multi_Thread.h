#ifndef _MULTI_THREAD_H_
#define _MULTI_THREAD_H_
#include <thread>
#include <list>
#include <mutex>
#include <iostream>
using namespace std;
typedef void (*FUNC)(void*);

class Job;
class JobGet;
class JobInsert;
class JobDelete;
class workLine;
class threadPool;
#endif

//单个管理员分配任务。稍加改造可以多个分配任务线程。


//工作及其方法类
class Job {
public:
	static list<int> pdata;
	static int count;
	static mutex mut2;

public:
	virtual void Run(void* data) = 0;		/*（先前）这里如果纯虚，就会无法定义类对象了*/
	static  bool setTypeofJob(workLine* w);	/*（之后）其实只用到这一个，一份。static就完了
												顺便整成了抽象类。
												纯虚函数，静态函数，静态变量。
											*/
};

list<int> Job::pdata;
int Job::count;
mutex Job::mut2;

//与线程绑定的类
class workLine {
public:
	Job* job = NULL;
	void* pd = NULL;
	mutex mut;
	condition_variable cond;//是为了P、V
	bool isGoNo = true, isEnd = true;
	void stopit() { isGoNo = false; }

	void operator()() {
		while (true) {
			unique_lock<mutex>u_lock(mut);
			cond.wait(u_lock, [this] {
				if (pd && job)return true;
				else if (!isGoNo)return true;//这句仅仅为了退出
				else return false;
				});

			if (!isGoNo)break;//这句仅仅为了退出

			isEnd = false;
			//不能放前面，否则死循环：忙-》进不去IdleT-》无法拿出赋值-》wait无法解除。

			job->Run(pd);	//执行。

			delete job, pd;
			pd = NULL, job = NULL;	/*一定要置空，否则无法判断*/

			isEnd = true;
		}
		cout << this_thread::get_id() << "线程已退出" << endl;
	}
};



class JobGet :public Job {
	void Run(void*) {
		unique_lock<mutex>ut_lock(mut2, try_to_lock);
		cout << this_thread::get_id() << "执行：getData" << endl;
		cout << "总数:" << count << endl;
		for (int& i : pdata)
		{
			cout << i << " ";
		}
		cout << endl;
	}
};
class JobInsert :public Job {
	void Run(void* data) {
		int* i = (int*)data;

		if (i) {
			unique_lock<mutex>ut_lock(mut2, try_to_lock);
			cout << this_thread::get_id() << "执行：insertData" << endl;
			pdata.push_front(*i);
			count++;
			//delete i;此处删除未置空，导致后面二次删除。
		}
	}
};
class JobDelete :public Job {
	void Run(void* data) {
		int* i = (int*)data;
		if (i) {
			unique_lock<mutex>ut_lock(mut2, try_to_lock);
			cout << this_thread::get_id() << "执行：deleteData" << endl;
			if (count > 0) {
				list<int>::iterator it = pdata.begin();
				while (it != pdata.end() && (*it) != (*i))it++;
				if (it != pdata.end()) {
					pdata.erase(it);
					count--;
				}
				else {
					cout << "无此值" << endl;
				}
			}
			else {
				cout << "队列为空，无此值" << endl;
			}
			//delete i;
		}
	}
};


bool Job::setTypeofJob(workLine* w) {
	cout << "选择：读取0，插入1，删除2" << endl;
	int No, dealNum;
	cin >> No;

	switch (No)
	{
	case 0:
		w->job = new JobGet();

		w->pd = new int(0);

		w->cond.notify_one();
		return true;

	case 1:
		w->job = new JobInsert();

		cin >> dealNum;
		w->pd = new int(dealNum);

		w->cond.notify_one();
		return true;

	case 2:

		w->job = new JobDelete();

		cin >> dealNum;
		w->pd = new int(dealNum);

		w->cond.notify_one();
		return true;

	default:
		cout << "哈哈，这次错了......" << endl;
		return false;
	}
}


//管理员 threadPool
class threadPool {
private:
	int tNum;
	list<thread>thrd;
	list<workLine*>idleT, busyT;//此处必须保存指针。
	//int idleNum, busyNum;
	//mutex dealList;
public:
	threadPool();
	~threadPool();

	void Run();					/*总执行线*/

	void addToIdle();
	workLine* getWorkLine();	/*此处有链表的查找删除*/
	void moveToBusyT(workLine* w);


};
threadPool::threadPool() {
	cout << "你想创建几个线程(0~100)" << endl;
	int n; cin >> n;

	if (100 > n&& n > 0) {

		tNum = n;

		while (n--)addToIdle();
	}

	Run();
}
void threadPool::Run() {

	bool gono = true;
	int times = 5;

	while (gono)
	{
		workLine* w = getWorkLine();

		if (Job::setTypeofJob(w))		/*此处不一定设置成功，因为输入可能非法，那就没必要转移了*/
			moveToBusyT(w);


		if (!(times--)) {
			cout << "你想停止吗？（Y|N）" << endl;
			char ch;	cin >> ch;
			if (ch == 'Y' || ch == 'y') {
				gono = false;
			}
			else
			{
				times = 5;
			}
		}
	}

	cout << "即将结束...." << endl;
}

void threadPool::addToIdle() {

	workLine* ptemp = new workLine();


	thread myt(ref(*ptemp));
	/*注意，此处用ref确保传入的是不是拷贝一次。
	！否则：考虑建立拷贝构造函数。*/

	thrd.push_front(move(myt));

	idleT.push_front(ptemp);	/*这里插进去的是变量值，所以其实是复制。变量ptemp在作用域后被回收*/
}

workLine* threadPool::getWorkLine() {
	while (true)
	{
		if (!idleT.empty()) {
			return  idleT.front();
		}
		else
		{
			for (list<workLine*>::iterator it = busyT.begin(), it2 = busyT.begin();
				it != busyT.end();)	//双指针确保删除不失效。
			{
				it2++;
				if ((*it)->isEnd) {
					idleT.push_front(*it);
					busyT.erase(it);
				}
				it = it2;
			}
		}
	}
}
void threadPool::moveToBusyT(workLine* w) {
	idleT.pop_front();
	busyT.push_front(w);
	//取用放在一起，防止任务假分配。	
}

threadPool::~threadPool() {
	//stopit放这儿还是放前面？
	for (workLine*& w : idleT) {
		w->stopit();
		w->cond.notify_one();
	}
	for (workLine*& w : busyT) {
		w->stopit();			//置停
		w->cond.notify_one();	//退出阻塞。
	}


	for (thread& temp : thrd) {
		temp.join();
	}

	for (workLine*& w : idleT) {
		if (w)delete w;
	}
	for (workLine*& w : busyT) {
		if (w)delete w;
	}
}