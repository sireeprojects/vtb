default:
	g++ -O3 -march=native backend.cpp -o backend `pkg-config --cflags --libs libdpdk`
	g++ -O3 -march=native frontend.cpp -o frontend `pkg-config --cflags --libs libdpdk`

clean:
	@rm -f backend frontend
