//Arbitrary thread maximum incase you want to bottleneck the simulation for testing (256 by default, because reasons)
#define MAX_THREADS 256u

//Includes
#include <iostream>
#include <algorithm>
#include <ctime>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>

//Custom randint function just to make things a little simpler 
int RandInt(int min, int max) {
	return min + rand() % ((max - min) + 1);
}

//Custom timer class, simply for the sake of keeping statistics.
class Timer {
public:
	Timer() {
		m_lastTime = std::chrono::steady_clock::now();
	}

	//A couple of restart functions, first saves/returns the elapsed time before restarting and the other simply restarts without returning anything.
	//T can be an int, float or double.
	template<class T>
	T Restart() {
		auto current = std::chrono::steady_clock::now();
		T totTime = std::chrono::duration<T>(current - m_lastTime).count();
		m_lastTime = current;
		return totTime;
	}

	void Restart() {
		m_lastTime = std::chrono::steady_clock::now();
	}

private:
	std::chrono::time_point<std::chrono::steady_clock> m_lastTime;
};

//Galton table class. Not sure why i made a class for it, but in case i wanted more at the same time...?
//unsigned ints are used becuase we dont want anything to be below zero.
class GaltonTable {
public:
	GaltonTable(unsigned int slotCount = 15, unsigned int ballCount = 100) : BallCount(ballCount), SlotCount(slotCount) {
		m_logicalCores = std::max<unsigned int>(std::thread::hardware_concurrency(), 1); //Retrieves the number of logical cores. harware_concurrency should return 0 if it is unable to retrieve a core count, but max keeps it at 1.
		m_logicalCores = std::min<unsigned int>(MAX_THREADS, m_logicalCores);
	}

	std::vector<unsigned int> Simulate() { //Returns a vector that represents how many balls have fallen into each slot
		m_slots = std::vector<unsigned int>(SlotCount, 0);
		
		for (size_t i = 0; i < m_logicalCores - 1; i++) {
			m_threads.push_back(std::thread(&GaltonTable::SimulationThread, this));
		}

		SimulationThread();

		for (size_t i = 0; i < m_logicalCores - 1; i++) {
			if (m_threads[i].joinable()) {
				m_threads[i].join();
			}
		}

		m_threads.clear();

		return m_slots;
	}

	void SimulationThread() {
		for (size_t i = 0; i < BallCount / m_logicalCores; i++) {
			int ballLocation = 0;
			for (size_t j = 0; j < m_slots.size() - 1; j++) {
				if (RandInt(0, 1) == 1 && ballLocation < m_slots.size() - 1) { //The other half of the statement is more as a safeguard incase the unlikely happens or if the board isnt big enough
					ballLocation++;
				}
			}
			m_slots[ballLocation]++;
		}
	}

	unsigned int SlotCount, BallCount;

private:
	unsigned int m_logicalCores;
	std::vector<unsigned int> m_slots;
	std::vector<std::thread> m_threads;
};

int main() {
	srand(time(0)); //Sets the seed for C rand() to the current time (a really, really long number. takes year, second, everything into account and is unique)
	Timer timer;

	while (true) {
		unsigned int slots;
		unsigned int balls;
		std::cout << "Input slots: ";
		std::cin >> slots;
		std::cout << "Input balls: ";
		std::cin >> balls;
		
		GaltonTable table = GaltonTable(slots, balls); //Creates a galton table object with the inputted varialbles.
		
		std::cout << "Simulating...\n";
		
		timer.Restart();
		std::vector<unsigned int> values = table.Simulate(); //Calls table's Simulate() function
		double SimTime = timer.Restart<float>();
		
		std::cout << "Finished Simulation! \n \n";
		//These lines find out the maximum number of balls there are in any of the slots (used to generate the graph)
		float maxValue = 0.f;
		for (size_t i = 0; i < values.size(); i++) {
			if (values[i] > maxValue)
				maxValue = values[i];
		}

		for (size_t i = 0; i < values.size(); i++) {
			//Creates graph
			int slotHeight = ((float)values[i] / maxValue) * 50 + 0.5;
			std::string row = "";
			for (size_t j = 0; j < slotHeight; j++) {
				row += "#";
			}
			printf("%s | %i, %f%% \n", row.c_str(), values[i], ((float)values[i] / (float)table.BallCount) * 100);
		}

		std::cout << "\nOperation took " << SimTime << " seconds, " << (SimTime / balls) * 1000.f << " milleseconds per ball.\n";
	}

	return 0;
}