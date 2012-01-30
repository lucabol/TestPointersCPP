// TestSmartPointerPolicies.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

using namespace std;

void printCond(string msg, function<bool ()> cond) {
	cout << setw(60) << msg << " : " << boolalpha << cond() << endl;
}

#define test(s, c) printCond(s, [&]() { return (c); });


struct NullDeleter {template<typename T> void operator()(T*) {} };
struct ArrayDeleter {template<typename T> void operator()(T* t) { delete[] t;}};


int main()
{
	cout << "Shared_ptr tests." << endl << endl;
	auto i1 = make_shared<int>(1);
	auto i11 = i1;
	auto i3 = make_shared<int>(3);
	auto i2 = make_shared<int>(2);
	auto i111 = make_shared<int>(1);
	auto imakewithnoargs = make_shared<int>();
	shared_ptr<int> inull0(0);
	shared_ptr<int> idefault;
	auto ires = make_shared<int>(10);
	auto iresPtr = ires.get();
	ires.reset();
	shared_ptr<int> ireleasable(new int(4), NullDeleter()); // wrong, it should work with an existing shared_ptr
	auto ireleasableRaw = ireleasable.get();
	ireleasable.reset();

	shared_ptr<int> ints(new int[10], ArrayDeleter());

	const auto ci = make_shared<int>(10);
	auto ic = make_shared<const int>(10);
	const auto cic = make_shared<const int>(10);

	auto f = [] (int* i) { return *i;};

	// shared_ptr have member functions (confusing with object member functions)
	cout << "Member funtion usage" << endl << endl;
	test("access raw pointer with get", *i1.get() == 1);
	test("is 2 is not less than 3 using owner_before", !i2.owner_before(i3));
	test("use_count gives the reference count", i1.use_count() == 2 && i3.use_count() == 1);
	test("unique tells you if this is the only manager", i3.unique() && !i1.unique());
	test("calling reset deletes memory", *iresPtr != 10);
	test("need decide releseable upfront", *ireleasableRaw == 4);

	cout << "Ownership handling strategy" << endl << endl;
	test("doesn't steal ownership with '='", *i1 == *i11);
	test("doesn't do deep copy", i11.get() == i1.get());
	test("doesn't implement '&' operator", true);
	test("doesn't implement a cast to pointee pointer", true);
	test("equality is pointer, not value, based", i111 != i1);
	test("less is also pointer based", i111 > i2);
	test("false if shared_ptr default constructor", !idefault);
	test("false if passing 0 to shared_ptr", !inull0);
	test("true if make_shared with no params", imakewithnoargs);
	test("cannot compare pointers of different types", true); // i1 == d1;
	test("no test for null ptr in init or deref (not even in debug)", !inull0); // *inull0; throws and shouldn't even been created in first place
	test("const pointee cannot be modified", true); // (*ic) = 3; throws
	test("array tracking is supported", shared_ptr<int>(new int[10], ArrayDeleter())); 

	delete ireleasableRaw;
	return 0;
}

