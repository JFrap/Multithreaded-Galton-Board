//Arbitrary thread maximum incase you want to bottleneck the simulation for testing (1024 by default, because reasons)
#define MAX_THREADS 1024u

//Bias for the bernoulli distribution (0 left, 0.5 middle, 1 right).
#define BIAS 0.5f

//Includes
#include <iostream>
#include <algorithm>
#include <ctime>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

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

	//Simply resets the timer.
	void Restart() {
		m_lastTime = std::chrono::steady_clock::now();
	}

private:
	std::chrono::time_point<std::chrono::steady_clock> m_lastTime; //The time restart() will be relative to.
};

//Galton table class. Not sure why i made a class for it, but in case i wanted more at the same time...?
//unsigned ints are used becuase we dont want anything to be below zero.
class GaltonTable {
public:
	GaltonTable(unsigned int slotCount = 15, unsigned int ballCount = 100) : BallCount(ballCount), SlotCount(slotCount) {
		if (ballCount > 1000) { //anything less than a thousand is hardly worth multithreading.
			m_logicalCores = std::max<unsigned int>(std::thread::hardware_concurrency(), 1); //Retrieves the number of logical cores. harware_concurrency should return 0 if it is unable to retrieve a core count, but max keeps it at 1.
			m_logicalCores = std::min<unsigned int>(MAX_THREADS, m_logicalCores);
		}
		else
			m_logicalCores = 1;

		m_rng.seed(std::random_device()());
		m_distribution = std::bernoulli_distribution(BIAS);
		SlotMutex = std::shared_ptr<std::mutex>(new std::mutex());
	}

	std::vector<unsigned int> Simulate() { //Returns a vector that represents how many balls have fallen into each slot
		m_slots = std::vector<unsigned int>(SlotCount, 0);

		//"m_logicalCores - 1" because one thread is already in use (for running the rest of the program), so we only start logical cores - 1 threads on seperate threads but we start one simulation on the current thread.
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
				if (m_distribution(m_rng) == 1 && ballLocation < m_slots.size() - 1) { //The other half of the statement is more as a safeguard incase the unlikely happens or if the board isnt big enough
					ballLocation++;
				}
			}
			SlotMutex->lock();
			m_slots[ballLocation]++; // F E A R L E S S C O N C U R R E N C Y
			SlotMutex->unlock();
		}
	}

	unsigned int SlotCount, BallCount;
	std::shared_ptr<std::mutex> SlotMutex;
private:
	unsigned int m_logicalCores;

	
	std::vector<unsigned int> m_slots;
	std::vector<std::thread> m_threads;

	std::mt19937 m_rng; //Random number generator
	std::bernoulli_distribution m_distribution; //Distribution. The bernoulli distribution is specialized for binary results, it isn't like a randint.
};

int main() {
	Timer timer;

	while (true) {
		unsigned int slots;
		unsigned int balls;
		std::cout << "Input slots: ";
		std::cin >> slots;
		std::cout << "Input balls: ";
		std::cin >> balls;

		//makes sure that you cant input anything that'll break anything
		slots = std::max<unsigned int>(slots, 3);
		balls = std::max<unsigned int>(balls, 1);

		GaltonTable table = GaltonTable(slots, balls); //Creates a galton table object with the inputted varialbles.

		std::cout << "Simulating...\n";

		timer.Restart();
		std::vector<unsigned int> values = table.Simulate(); //Calls table's Simulate() function
		double SimTime = timer.Restart<float>();

		std::cout << "Finished Simulation! \n \n";

		//These lines find out the maximum number of balls there are in any of the slots (used to generate the graph)
		float maxValue = 0.f;
		unsigned int totValue = 0;
		for (size_t i = 0; i < values.size(); i++) {
			totValue += values[i];
			if (values[i] > maxValue)
				maxValue = (float)values[i];
		}

		//Creates graph
		for (size_t y = 0; y < 20; y++) {
			for (size_t x = 0; x < values.size(); x++) {
				int slotHeight = (int)(((float)values[x] / maxValue) * 20 + 0.5f); //0.5 because we want to round up/down to the nearest integer, casting to an int floors (rounds down) it.
				if (20 - y <= slotHeight) {
					printf("| ");
				}
				else printf("  ");
			}
			printf("\n");
		}

		//Prints out index, number of balls and percentage
		for (size_t i = 0; i < values.size(); i++) {
			printf("Column %i | %i, %f%% \n", (int)i, values[i], ((float)values[i] / (float)table.BallCount) * 100);
		}

		//Final statistics.
		std::cout << "\nOperation took " << SimTime << " seconds, " << (SimTime / balls) * 1000.f << " milleseconds per ball with " << totValue << " balls\n";
	}

	return 0;
}
