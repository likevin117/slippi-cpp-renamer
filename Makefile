CXX = c++
CXXFLAGS = -std=c++17 -Wall -Wextra

TARGET = slippi-parser
SRC = main.cpp

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
