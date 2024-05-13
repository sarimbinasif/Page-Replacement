#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>

using namespace std;

vector<int> frame;           // Frame to hold memory pages
int pointer = 0;             // Pointer to track the next page to be replaced
int faults = 0;              
int hits = 0;                
mutex mtx;                   

void print(int frame_size, const vector<int>& timestamp = {}) {
    cout << "Printing the Frames: ";
    for (int i = 0; i < frame_size; i++) {
        if (frame[i] == -1)
            cout << "- ";
        else
            cout << frame[i] << " ";
        if (!timestamp.empty()) {
            cout << "(" << timestamp[i] << ") ";
        }
    }
    cout << endl;
}


void FIFO(int reference, int frame_size) {
    lock_guard<mutex> lock(mtx);
    bool alloted = false;
    for (int i = 0; i < frame_size; ++i) {
        if (frame[i] == reference) {
            alloted = true;
            cout << "  Hit for " << reference << " | ";
            hits++;
            break;
        } else if (frame[i] == -1) {
            alloted = true;
            frame[i] = reference;
            cout << "Fault for " << reference << " | ";
            faults++;
            break;
        }
    }
    if (!alloted) {
        faults++;
        cout << "Fault for " << reference << " | ";
        frame[pointer] = reference;
        pointer = (pointer + 1) % frame_size;
    }
    print(frame_size);
}


void LRU(int reference, int frame_size, vector<int>& timestamp) {
    lock_guard<mutex> lock(mtx);
    bool alloted = false;
    int least_used_index = 0;
    for (int i = 0; i < frame_size; ++i) {
        if (frame[i] == reference) {
            alloted = true;
            cout << "  Hit for " << reference << " | ";
            hits++;
            timestamp[i] = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
            break;
        } else if (frame[i] == -1) {
            alloted = true;
            frame[i] = reference;
            timestamp[i] = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
            cout << "Fault for " << reference << " | ";
            faults++;
            break;
        } else if (timestamp[i] < timestamp[least_used_index]) {
            least_used_index = i;
        }
    }
    if (!alloted) {
        faults++;
        cout << "Fault for " << reference << " | ";
        frame[least_used_index] = reference;
        timestamp[least_used_index] = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
    }
    print(frame_size, timestamp);
}

void optimal(int reference, int frame_size, const vector<int>& references, int current_index) {
    lock_guard<mutex> lock(mtx);
    bool alloted = false;
    int farthest_index = -1;
    int farthest = current_index;  // Initialize farthest distance to a negative value
    for (int i = 0; i < frame_size; i++) {
        if (frame[i] == reference) {
            alloted = true;
            cout << "  Hit for " << reference << " | ";
            hits++;
            break;
        } else if (frame[i] == -1) {
            alloted = true;
            frame[i] = reference;
            cout << "Fault for " << reference << " | ";
            faults++;
            break;
        }
    }
    	
    if (!alloted) {
    	for(int i=0; i < frame_size; i++){
            int j;
            for (j = current_index + 1; j < references.size(); j++) {
                if (references[j] == frame[i]) {
                    if (j > farthest) {
	                    farthest = j;
	                    farthest_index = i;
					}
					break;
                }    
            }
            
            if(j == references.size()){
            	farthest_index = i;
            	break;
			}
		}
			
		if(farthest_index == -1)
			farthest_index = 0;
        faults++;
        cout << "Fault for " << reference << " | ";
        frame[farthest_index] = reference;
    }
    print(frame_size);
}

void page_reference_handler(int frame_size, vector<int>& references, int start, int end, const string& algorithm, vector<int>& timestamp) {
    if (algorithm == "FIFO") {
        for (int i = start; i < end; i++) {
            FIFO(references[i], frame_size);
        }
    } else if (algorithm == "LRU") {
        for (int i = start; i < end; i++) {
            LRU(references[i], frame_size, timestamp);
        }
    } else if (algorithm == "Optimal") {
        for (int i = start; i < end; i++) {
            optimal(references[i], frame_size, references, i);
        }
    }
}

int main() {
    int frame_size, number_of_references, num_threads;
    string algorithm;

    cout << "Enter frame size: ";
    cin >> frame_size;

    cout << "Enter the number of references: ";
    cin >> number_of_references;

    cout << "Enter the number of threads: ";
    cin >> num_threads;

    cout << "Enter the algorithm (FIFO, LRU, or Optimal): ";
    cin >> algorithm;

    frame.resize(frame_size, -1);

    print(frame_size);

    vector<int> references(number_of_references);
    vector<int> timestamp(frame_size, 0); // Timestamp for LRU algorithm

    cout << "Enter the references: ";
    for (int i = 0; i < number_of_references; i++) {
        cin >> references[i];
    }


    vector<thread> threads;
    int chunk_size = number_of_references / num_threads;
    int remaining = number_of_references % num_threads;
    int start = 0, end;

    for (int i = 0; i < num_threads; i++) {
        end = start + chunk_size + (i < remaining ? 1 : 0);
        threads.emplace_back(page_reference_handler, frame_size, ref(references), start, end, algorithm, ref(timestamp));
        start = end;
    }

    for (auto& th : threads) {
        th.join();
    }

    cout << "\nNumber of faults: " << faults << "\nNumber of hits: " << hits << endl;

    return 0;
}