#include "stdafx.h"

using namespace std;

#define bigBlock 20
int howMany = 1000;
int filterNo = 500;

struct Record {
    int Id;
    char k1[20];
    char k2[20];
    char k3[20];
    char k4[20];
    char k5[20];
    char k6[20];
    char mem[bigBlock];
	void Lock() {}
	void Unlock() {}
};

//// multithread refcount no lock on object (as shared_ptr)
//typedef Loki::SmartPtr<Record,
//				 Loki::RefCountedMTAdj<LOKI_DEFAULT_THREADING_NO_OBJ_LEVEL>::RefCountedMT,
//				 Loki::DisallowConversion,
//			     Loki::AssertCheckStrict,
//				 Loki::DefaultSPStorage,
//				 LOKI_DEFAULT_CONSTNESS> RecordPtr;

//// fully multithread with lock of object
//typedef Loki::SmartPtr<Record,
//				 Loki::RefCountedMTAdj<LOKI_DEFAULT_THREADING_NO_OBJ_LEVEL>::RefCountedMT,
//				 Loki::DisallowConversion,
//			     Loki::AssertCheckStrict,
//				 Loki::LockedStorage,
//				 LOKI_DEFAULT_CONSTNESS> RecordPtr;

// standard single threaded
typedef Loki::SmartPtr<Record,
				 Loki::RefCounted,
				 Loki::DisallowConversion,
			     Loki::AssertCheckStrict,
				 Loki::DefaultSPStorage,
				 LOKI_DEFAULT_CONSTNESS> RecordPtr;

template<class T>
vector<T> filter(const vector<T>& v, function<bool (const T&)> f) {
    vector<T> v1;
    for_each(v.begin(), v.end(), [&](T r) { 
              if(f(r)) v1.push_back(r);});
    return v1;
}

template<class T>
int sum(const vector<T>& v, function<int (const T&)> f) {
    int sum = 0;
    for_each(v.begin(), v.end(), [&](T r) { 
              sum += f(r);});
    return sum;
}

vector<Record*> filter(const vector<Record*>& v, function<bool (Record*)> f) {
    vector<Record*> v1;
    for_each(v.begin(), v.end(), [&](Record* r) { 
              if(f(r)) v1.push_back(r);});
    return v1;
}

int sum(const vector<Record*>& v, function<int (Record*)> f) {
    int sum = 0;
    for_each(v.begin(), v.end(), [&](Record* r) { 
              sum += f(r);});
    return sum;
}

int normal() {

    vector<Record> v;
    for (int i = 0; i < howMany; ++i) {
        Record r;
        r.Id = i;
        strcpy(r.k1, "0123456789");
        strcpy(r.k2, "0123456789");
        strcpy(r.k3, "0123456789");
        strcpy(r.k4, "0123456789");
        strcpy(r.k5, "0123456789");
        strcpy(r.k6, "0123456789");
        memset(r.mem, '-', bigBlock);
        v.push_back(r);
    }

    function<bool (const Record&)> f = [](const Record& r) {return r.Id < filterNo;};
    function<int (const Record&)> s = [] (const Record& r) { return r.Id;};

    return sum(filter(v,f),s);
}

typedef enum {Normal, Boost, Make, LokiPtr} WhichOne;

struct RecordTag {};
typedef boost::singleton_pool<RecordTag, sizeof(Record)> record_pool;

int pointers(WhichOne which) {

    vector<shared_ptr<Record>> v;
    for (int i = 0; i < howMany; ++i) {
        shared_ptr<Record> r;
        if(which == Normal)
            r = shared_ptr<Record>(new Record);
        else if(which == Boost)
            r = shared_ptr<Record>((Record*)record_pool::malloc(), [](void* p) {record_pool::free(p);});
        else if(which == Make)
            r = make_shared<Record>();
		else throw "This kind of pointer doesn't exist";

        r->Id = i;
        strcpy(r->k1, "0123456789");
        strcpy(r->k2, "0123456789");
        strcpy(r->k3, "0123456789");
        strcpy(r->k4, "0123456789");
        strcpy(r->k5, "0123456789");
        strcpy(r->k6, "0123456789");
        memset(r->mem, '-', bigBlock);
        v.push_back(r);
    }

    function<bool (const shared_ptr<Record>&)> f = [](const shared_ptr<Record>& r) {return r->Id < filterNo;};
    function<int (const shared_ptr<Record>&)> s = [] (const shared_ptr<Record>& r) { return r->Id;};
    int ret = sum(filter(v,f), s);
    return ret;
}

int lokipointers(WhichOne which) {
    vector<RecordPtr> v;
    for (int i = 0; i < howMany; ++i) {
		Record* ptr;
        RecordPtr r;  
        if(which == Normal)
            r = RecordPtr(new Record());
        //else if(which == Boost)
        //    r = RecordPtr((Record*)record_pool::malloc(), [](void* p) {record_pool::free(p);});
        //else if(which == Make)
        //    r = make_shared<Record>();
		else throw "This kind of pointer doesn't exist";

        r->Id = i;
        strcpy(r->k1, "0123456789");
        strcpy(r->k2, "0123456789");
        strcpy(r->k3, "0123456789");
        strcpy(r->k4, "0123456789");
        strcpy(r->k5, "0123456789");
        strcpy(r->k6, "0123456789");
        memset(r->mem, '-', bigBlock);
        v.push_back(r);
    }

    function<bool (const RecordPtr&)> f = [](const RecordPtr& r) {return static_cast<Record*>(Loki::GetImpl(r))->Id < filterNo;};
    function<int (const RecordPtr&)> s = [] (const RecordPtr& r) { return static_cast<Record*>(Loki::GetImpl(r))->Id;};
    int ret = sum(filter(v,f), s);
    return ret;

}


int boostallocator(WhichOne which) {

    boost::pool<> p(sizeof(Record));
    vector<Record*> v;
    Record* r;
    for (int i = 0; i < howMany; ++i) {
        if(which == Boost)
            r = (Record*)p.malloc(); // memory freed at function exit
        else
            r = new Record; // oops, memory leak
        r->Id = i;
        strcpy(r->k1, "0123456789");
        strcpy(r->k2, "0123456789");
        strcpy(r->k3, "0123456789");
        strcpy(r->k4, "0123456789");
        strcpy(r->k5, "0123456789");
        strcpy(r->k6, "0123456789");
        memset(r->mem, '-', bigBlock);
        v.push_back(r);
    }

    function<bool ( Record*)> f = [](Record* r) {return r->Id < filterNo;};
    function<int ( Record*)> s = [] (Record* r) { return r->Id;};

    return sum(filter(v,f), s);
}

int doTest(function<int ()> f) {
    int ret = 0;
    const int repetitions = 1000;
       clock_t start, finish;
       clock_t duration;

    start = clock();
    for(int i = 0; i < repetitions; ++i)
        ret += f();
       finish = clock();
    duration = (finish - start)  * 1000 / CLOCKS_PER_SEC;
    std::cout << "value: " << ret << endl;
	return duration;
}

int main()
{
       cout << "The size of the allocated object is " << sizeof(Record) << endl;
	   typedef tuple<clock_t, string> ResultType;
	   vector<ResultType> durations;

	   durations.push_back(make_tuple(doTest([]() { return normal();}), "Stack allocation"));
	   durations.push_back(make_tuple(doTest([]() { return boostallocator(Normal);}), "Heap allocation (no free)"));
	   durations.push_back(make_tuple(doTest([]() { return pointers(Normal);}), "Shared_ptr - constructor"));
	   durations.push_back(make_tuple(doTest([]() { return pointers(Make);}), "Shared_ptr - make_shared"));
	   durations.push_back(make_tuple(doTest([]() { return lokipointers(Normal);}), "Loki shared pointers"));
	   durations.push_back(make_tuple(doTest([]() { return pointers(Boost);}), "Shared_ptr with pooled allocator"));
	   durations.push_back(make_tuple(doTest([]() { return boostallocator(Boost);}), "Pooled allocator"));

	   sort(durations.begin(), durations.end(), [](ResultType r1, ResultType r2) { return (get<0>(r1) < get<0>(r2));});
	   for_each(durations.begin(), durations.end(), [](ResultType r) { cout << setw(40) << get<1>(r) << ": " << get<0>(r) << endl;}); 
       return 0;
}




