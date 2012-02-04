#include "stdafx.h"

#include <boost\pool\pool.hpp>
#include <boost\pool\singleton_pool.hpp>
#include <loki\smartptr.h>
#include <stlsoft\containers\pod_vector.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/numeric.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/adaptor/indirected.hpp>
#include <boost/chrono/process_cpu_clocks.hpp>
#include <boost/chrono/time_point.hpp>
#include <boost/chrono/chrono_io.hpp>

using namespace std;
using stlsoft::pod_vector;
using namespace boost::chrono;
using boost::adaptors::filtered;
using boost::adaptors::transformed;
using boost::adaptors::indirected;

const int repetitions = 1000;
const int bigBlock = 26;
const int howMany = 1000;
const int filterNo = 100;

typedef boost::chrono::process_cpu_clock the_clock;

struct timer {
	timer(): clock_(the_clock::now()) {}

	the_clock::times elapsed() {
		return (the_clock::now() - clock_).count();
	}

	the_clock::time_point clock_;
};

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
			     Loki::AssertCheck,
				 Loki::DefaultSPStorage,
				 LOKI_DEFAULT_CONSTNESS> RecordPtr;


struct to_int {
	typedef int result_type;
	int operator() (const Record& r) const {
		return r.Id;
	};
};

function<bool (const Record&)> filter_f = [](const Record& r) {return r.Id < filterNo;};

template <class Range>
int accumulate_filter(const Range& r) {
	return boost::accumulate(
		r | filtered(filter_f) | transformed(to_int()),
		0,
		plus<int>());
}

void record_init(Record& r, int i) {

    r.Id = i;
    strcpy(r.k1, "0");
    strcpy(r.k2, "0");
    strcpy(r.k3, "0");
    strcpy(r.k4, "0");
    strcpy(r.k5, "0");
    strcpy(r.k6, "0");
    memset(r.mem, '-', bigBlock);

}

int normal() {

    vector<Record> v;
    for (int i = 0; i < howMany; ++i) {
        Record r;
		record_init(r, i);
        v.push_back(r);
    }

	return accumulate_filter(v);
}

template<int size>
int podVector() {

    pod_vector<Record, stlsoft::allocator_selector<Record>::allocator_type, size> v;
    for (int i = 0; i < howMany; ++i) {
        Record r;
		record_init(r, i);
        v.push_back(r);
    }

	return accumulate_filter(v);
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

		record_init(*r, i);
        v.push_back(r);
    }

	return accumulate_filter(v | indirected);
}

int lokipointers(WhichOne) {
    vector<RecordPtr> v;
    for (int i = 0; i < howMany; ++i) {
        RecordPtr r = RecordPtr(new Record());
		record_init(*r, i);
        v.push_back(r);
    }

	auto ret = accumulate_filter(v | transformed(RecordPtr::GetPointer) | indirected);
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

		record_init(*r, i);
        v.push_back(r);
    }

	return accumulate_filter(v | indirected);
}

int doTest(function<int ()> f) {
    int ret = 0;

	timer timer;
    for(int i = 0; i < repetitions; ++i)
        ret += f();

    auto el = timer.elapsed();
    std::cout << "value: " << ret << endl;
	return el.system + el.user;
}

int main()
{
       cout << "Size of object : " << sizeof(Record) << endl;
       cout << "Num. of objects: " << howMany << endl;
	   typedef tuple<clock_t, string> ResultType;
	   vector<ResultType> durations;

	   cout << endl;
	   durations.push_back(make_tuple(doTest([]() { return normal();}), "Stack allocation"));
	   durations.push_back(make_tuple(doTest([]() { return podVector<howMany*2>();}), "Stack allocation (pod_vector right size)"));
	   durations.push_back(make_tuple(doTest([]() { return podVector<howMany/10>();}), "Stack allocation (pod_vector wrong size)"));
	   durations.push_back(make_tuple(doTest([]() { return boostallocator(Normal);}), "Heap allocation (no free)"));
	   durations.push_back(make_tuple(doTest([]() { return pointers(Normal);}), "Shared_ptr - constructor"));
	   durations.push_back(make_tuple(doTest([]() { return pointers(Make);}), "Shared_ptr - make_shared"));
	   // Loki throws an exception on main exit, not worried because I'm not going to use it anyhow
	   durations.push_back(make_tuple(doTest([]() { return lokipointers(Normal);}), "Loki shared pointers"));
	   durations.push_back(make_tuple(doTest([]() { return pointers(Boost);}), "Shared_ptr with pooled allocator"));
	   durations.push_back(make_tuple(doTest([]() { return boostallocator(Boost);}), "Pooled allocator"));

	   //sort(durations.begin(), durations.end(), [](ResultType r1, ResultType r2) { return (get<0>(r1) < get<0>(r2));});
	   for_each(durations.begin(), durations.end(), [](ResultType r) { cout /*<< setw(40) << get<1>(r) << ": " */<< get<0>(r) << endl;});
       return 0;
}




