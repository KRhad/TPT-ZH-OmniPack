#ifndef TPT_RAND_
#define TPT_RAND_

#include <array>
#include <stdint.h>
#include "Singleton.h"

class RNG : public Singleton<RNG>
{
public:
	using State = std::array<uint64_t, 2>;

private:
	State s;
	uint64_t next();

public:
	unsigned int operator()();
	unsigned int gen();
	int between(int lower, int upper);
	bool chance(int numerator, unsigned int denominator);
	float uniform01();

	RNG();
	void seed(unsigned int sd);

	void state(State ns)
	{
		s = ns;
	}

	State state() const
	{
		return s;
	}
};

#endif /* TPT_RAND_ */
