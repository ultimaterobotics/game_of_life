###
Name := game_of_life
CXX := g++
CXXFLAGS := -O3 -Wall
Libs := -lSDL2 -lSDL2_ttf 

$(Name): main.cpp
	$(CXX) -o $(Name) main.cpp $(Libs) $(CXXFLAGS)

clean: 
	rm $(Name)
