

sha3_headers = Endian.h HashFunction.h Keccak.h CommandParser.h
sha3_sources = HashFunction.cpp  Keccak.cpp  CommandParser.h
sha3_deps = $(sha3_headers) $(sha3_sources)

sha3sum: $(sha3_deps)
	g++ -o sha3sum -O3 -std=c++0x $(sha3_sources)

