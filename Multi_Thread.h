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

//��������Ա���������ԼӸ�����Զ�����������̡߳�


//�������䷽����
class Job {
public:
	static list<int> pdata;
	static int count;
	static mutex mut2;

public:
	virtual void Run(void* data) = 0;		/*����ǰ������������飬�ͻ��޷������������*/
	static  bool setTypeofJob(workLine* w);	/*��֮����ʵֻ�õ���һ����һ�ݡ�static������
												˳�������˳����ࡣ
												���麯������̬��������̬������
											*/
};

list<int> Job::pdata;
int Job::count;
mutex Job::mut2;

//���̰߳󶨵���
class workLine {
public:
	Job* job = NULL;
	void* pd = NULL;
	mutex mut;
	condition_variable cond;//��Ϊ��P��V
	bool isGoNo = true, isEnd = true;
	void stopit() { isGoNo = false; }

	void operator()() {
		while (true) {
			unique_lock<mutex>u_lock(mut);
			cond.wait(u_lock, [this] {
				if (pd && job)return true;
				else if (!isGoNo)return true;//������Ϊ���˳�
				else return false;
				});

			if (!isGoNo)break;//������Ϊ���˳�

			isEnd = false;
			//���ܷ�ǰ�棬������ѭ����æ-������ȥIdleT-���޷��ó���ֵ-��wait�޷������

			job->Run(pd);	//ִ�С�

			delete job, pd;
			pd = NULL, job = NULL;	/*һ��Ҫ�ÿգ������޷��ж�*/

			isEnd = true;
		}
		cout << this_thread::get_id() << "�߳����˳�" << endl;
	}
};



class JobGet :public Job {
	void Run(void*) {
		unique_lock<mutex>ut_lock(mut2, try_to_lock);
		cout << this_thread::get_id() << "ִ�У�getData" << endl;
		cout << "����:" << count << endl;
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
			cout << this_thread::get_id() << "ִ�У�insertData" << endl;
			pdata.push_front(*i);
			count++;
			//delete i;�˴�ɾ��δ�ÿգ����º������ɾ����
		}
	}
};
class JobDelete :public Job {
	void Run(void* data) {
		int* i = (int*)data;
		if (i) {
			unique_lock<mutex>ut_lock(mut2, try_to_lock);
			cout << this_thread::get_id() << "ִ�У�deleteData" << endl;
			if (count > 0) {
				list<int>::iterator it = pdata.begin();
				while (it != pdata.end() && (*it) != (*i))it++;
				if (it != pdata.end()) {
					pdata.erase(it);
					count--;
				}
				else {
					cout << "�޴�ֵ" << endl;
				}
			}
			else {
				cout << "����Ϊ�գ��޴�ֵ" << endl;
			}
			//delete i;
		}
	}
};


bool Job::setTypeofJob(workLine* w) {
	cout << "ѡ�񣺶�ȡ0������1��ɾ��2" << endl;
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
		cout << "��������δ���......" << endl;
		return false;
	}
}


//����Ա threadPool
class threadPool {
private:
	int tNum;
	list<thread>thrd;
	list<workLine*>idleT, busyT;//�˴����뱣��ָ�롣
	//int idleNum, busyNum;
	//mutex dealList;
public:
	threadPool();
	~threadPool();

	void Run();					/*��ִ����*/

	void addToIdle();
	workLine* getWorkLine();	/*�˴�������Ĳ���ɾ��*/
	void moveToBusyT(workLine* w);


};
threadPool::threadPool() {
	cout << "���봴�������߳�(0~100)" << endl;
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

		if (Job::setTypeofJob(w))		/*�˴���һ�����óɹ�����Ϊ������ܷǷ����Ǿ�û��Ҫת����*/
			moveToBusyT(w);


		if (!(times--)) {
			cout << "����ֹͣ�𣿣�Y|N��" << endl;
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

	cout << "��������...." << endl;
}

void threadPool::addToIdle() {

	workLine* ptemp = new workLine();


	thread myt(ref(*ptemp));
	/*ע�⣬�˴���refȷ��������ǲ��ǿ���һ�Ρ�
	�����򣺿��ǽ����������캯����*/

	thrd.push_front(move(myt));

	idleT.push_front(ptemp);	/*������ȥ���Ǳ���ֵ��������ʵ�Ǹ��ơ�����ptemp��������󱻻���*/
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
				it != busyT.end();)	//˫ָ��ȷ��ɾ����ʧЧ��
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
	//ȡ�÷���һ�𣬷�ֹ����ٷ��䡣	
}

threadPool::~threadPool() {
	//stopit��������Ƿ�ǰ�棿
	for (workLine*& w : idleT) {
		w->stopit();
		w->cond.notify_one();
	}
	for (workLine*& w : busyT) {
		w->stopit();			//��ͣ
		w->cond.notify_one();	//�˳�������
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