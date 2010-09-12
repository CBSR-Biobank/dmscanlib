
#include "ProfileSettings.h"
#include "UaAssert.h"

ProfileSettings::ProfileSettings(unsigned word1, unsigned word2, unsigned word3) :
	bits(NUM_CELLS) {

	unsigned words[NUM_WORDS] = { word1, word2, word3 };

	unsigned mask;
	for (unsigned i = 0; i < NUM_WORDS; ++i) {
		mask = 1;
		for (unsigned j = 0; j < 32; ++j) {
			if (words[i] & mask) {
				bits[32 * i + j] = 1;
			}
			mask <<= 1;
		}
	}
}

bool ProfileSettings::operator[] (unsigned x) {
	UA_ASSERTS(x < NUM_CELLS, "invalid bit requested " << x);
	return bits[x];
}
