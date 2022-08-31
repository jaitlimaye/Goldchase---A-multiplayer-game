goldchase: goldchase.cpp libmap.a goldchase.h libmap.a
	g++ -std=c++17 goldchase.cpp -o goldchase -L. -lmap -lpanel -lncurses -pthread -lrt 

libmap.a: Screen.o Map.o Game.o
	ar -r libmap.a Screen.o Map.o Game.o

Game.o: Game.cpp
	g++ -std=c++17 -c Game.cpp

Screen.o: Screen.cpp
	g++ -std=c++17 -c Screen.cpp

Map.o: Map.cpp
	g++ -std=c++17 -c Map.cpp

clean:
	rm -f Screen.o Map.o libmap.a goldchase Game.o
