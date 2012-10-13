#include <sys/time.h>
#include <iostream>

#ifndef TIMEVALOPERATORS_H
#define TIMEVALOPERATORS_H

using namespace std;

class timeval_operator_exception: public exception {
};

// ***** output *****
inline ostream& operator<<(ostream& out,struct timeval tv) {
	out << "sec: " << tv.tv_sec << " usec: " << tv.tv_usec;
	return out;
}

// ***** comparison *****
inline bool operator<(struct timeval t0,struct timeval t1) {
	return (t0.tv_sec < t1.tv_sec)
		|| (t0.tv_sec == t1.tv_sec && t0.tv_usec < t1.tv_usec);
}
inline bool operator<=(struct timeval t0,struct timeval t1) {
	return (t0.tv_sec < t1.tv_sec)
		|| (t0.tv_sec == t1.tv_sec && t0.tv_usec <= t1.tv_usec);
}
inline bool operator==(struct timeval t0,struct timeval t1) {
	return t0.tv_sec == t1.tv_sec && t0.tv_usec == t1.tv_usec;
}
inline bool operator!=(struct timeval t0,struct timeval t1) {
	return !(t0 == t1);
}
inline bool operator>(struct timeval t0,struct timeval t1) {
	return !(t0 <= t1);
}
inline bool operator>=(struct timeval t0,struct timeval t1) {
	return !(t0 < t1);
}

// ***** arithmetic *****
inline struct timeval operator+(struct timeval t0,struct timeval t1) {
	struct timeval t = {t0.tv_sec+t1.tv_sec,t0.tv_usec+t1.tv_usec};
	if (t.tv_usec >= 1000000) { // carry needed
		t.tv_sec++;
		t.tv_usec -= 1000000;
	}
	return t;
}
inline struct timeval operator+=(struct timeval& t0,struct timeval t1) {
	t0.tv_sec += t1.tv_sec;
	t0.tv_usec += t1.tv_usec;
	if (t0.tv_usec >= 1000000) { // carry needed
		t0.tv_sec++;
		t0.tv_usec -= 1000000;
	}
	return t0;
}
inline struct timeval operator-(struct timeval t0,struct timeval t1) {
	if (t0 < t1) {
		throw timeval_operator_exception();
	}
	struct timeval t = {t0.tv_sec-t1.tv_sec,t0.tv_usec-t1.tv_usec};
	if (t.tv_usec < 0) { // borrow needed
		t.tv_sec--;
		t.tv_usec += 1000000;
	}
	return t;
}
inline struct timeval operator-=(struct timeval& t0,struct timeval t1) {
	if (t0 < t1) {
		throw timeval_operator_exception();
	}
	t0.tv_sec -= t1.tv_sec;
	t0.tv_usec -= t1.tv_usec;
	if (t0.tv_usec < 0) { // borrow needed
		t0.tv_sec--;
		t0.tv_usec += 1000000;
	}
	return t0;
}

#endif
