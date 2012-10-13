set errexit

echo ---------- compiling physical_layer.cpp
g++ -g -c -Wall physical_layer.cpp

echo ---------- compiling link_layer.cpp
g++ -g -c -Wall link_layer.cpp

echo ---------- compiling link_layer_test.cpp
g++ -g -c -Wall link_layer_test.cpp

echo ---------- linking
g++ -g -o link_layer_test \
	physical_layer.o link_layer.o link_layer_test.o -lpthread
