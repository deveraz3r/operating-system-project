#include <iostream>
#include <pthread.h>
#include <queue>
#include <vector>
#include <string>
#include <unistd.h>
using namespace std;

// Global Variables
int bricks = 10;
int cement = 10;
bool isCompleted = false;   //notify all process the completion of tasks.

// Mutex for resource synchronization
pthread_mutex_t resource_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t wheel_cart_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t task_mutex = PTHREAD_MUTEX_INITIALIZER;

// Task Queues
queue<string> high_priority_queue;
queue<string> medium_priority_queue;
queue<string> low_priority_queue;

//Skill enum
enum Skill {
  LOW,
  MEDIUM,
  HIGH,
};

// Worker Data
struct Worker {
    string name;
    Skill skill;
    int energy = 100; // Full energy is 100
    bool isWorking = false;
};
vector<Worker> workers;

//Helper Functions
void initialize_resources() {
    cout << "Resources initialized.\n";
}

void initialize_workers() {
    workers.push_back({"Worker-1", Skill::HIGH, 100, false});
    workers.push_back({"Worker-2", Skill::MEDIUM, 100, false});
    workers.push_back({"Worker-3", Skill::LOW, 100, false});
    workers.push_back({"Worker-4", Skill::LOW, 100, false});
    workers.push_back({"Worker-5", Skill::MEDIUM, 100, false});
    cout << "Workers initialized.\n";
}

void initialize_tasks() {
    for (int i = 0; i < 5; ++i) high_priority_queue.push("Foundation laying");
    for (int i = 0; i < 10; ++i) medium_priority_queue.push("General construction");
    for (int i = 0; i < 5; ++i) low_priority_queue.push("Finishing touches");
    cout << "Tasks initialized.\n";
}

// Task Scheduling with Task Mutex
string assign_task(Worker* worker) {
    pthread_mutex_lock(&task_mutex); // Lock task mutex

    if (!high_priority_queue.empty() && worker->skill==Skill::HIGH) {
        string task = high_priority_queue.front();
        high_priority_queue.pop();
        pthread_mutex_unlock(&task_mutex);
        return task;
    }
    if (!medium_priority_queue.empty() && worker->skill==Skill::MEDIUM) {
        string task = medium_priority_queue.front();
        medium_priority_queue.pop();
        pthread_mutex_unlock(&task_mutex);
        return task;
    }
    if (!low_priority_queue.empty() && worker->skill==Skill::LOW) {
        string task = low_priority_queue.front();
        low_priority_queue.pop();
        pthread_mutex_unlock(&task_mutex);
        return task;
    }

    isCompleted = true; //all tasks are completed
    pthread_mutex_unlock(&task_mutex);
    return "";
}

// Resource Management
bool allocate_resources() {
    pthread_mutex_lock(&resource_mutex);

    // Check if resources are available
    if (bricks > 0 && cement > 0) {
        // Use the wheel cart (synchronized with its mutex)
        pthread_mutex_lock(&wheel_cart_mutex);
        cout << "Using wheel_cart to transfer 1 unit of bricks and cement.\n";

        --bricks;
        --cement;

        cout << "Allocated 1 unit of bricks and cement. Remaining: Bricks = " << bricks
             << ", Cement = " << cement << ".\n";
        pthread_mutex_unlock(&wheel_cart_mutex);
        pthread_mutex_unlock(&resource_mutex);
        return true;
    }

    pthread_mutex_unlock(&resource_mutex);
    return false;
}

void* replenish_resources(void *) {
    while (!isCompleted) {
        sleep(5); // Simulate replenishment delay
        pthread_mutex_lock(&resource_mutex);
        bricks += 5; // Add 5 units of bricks
        cement += 5; // Add 5 units of cement
        cout << "Resources replenished: Bricks = " << bricks << ", Cement = " << cement << ".\n";
        pthread_mutex_unlock(&resource_mutex);
    }
    return NULL;
}

// Worker Behavior
void* simulate_worker_behavior(void* arg) {
    Worker* worker = (Worker*)arg;

    while (!isCompleted) {
        if (worker->energy <= 0) {
            cout << worker->name << " is on a break to restore energy.\n";
            sleep(3); // Simulate break time
            worker->energy = 100; // Fully restored energy
            continue;
        }

        string task = assign_task(worker);
        if (task.empty()) {
            worker->isWorking = false;
            sleep(1); // Wait for new tasks
            continue;
        }

        worker->isWorking = true;
        if (allocate_resources()) {
            cout << worker->name << " is performing task: " << task << ".\n";
            sleep(2); // Simulate task duration
            worker->energy -= 20; // Decrease energy after completing the task
            cout << worker->name << " completed task: " << task << ".\n";
        } else  {
            // Re-add task to the queue if resources are unavailable
            pthread_mutex_lock(&task_mutex);
            if (task == "Foundation laying") {
                high_priority_queue.push(task);
            } else if (task == "General construction") {
                medium_priority_queue.push(task);
            } else if (task == "Finishing touches") {
                low_priority_queue.push(task);
            }
            pthread_mutex_unlock(&task_mutex);
            
            cout << worker->name << " waiting for resources to perform task: " << task << ".\n";
            sleep(2); // Wait for resources
            continue;
        }
    }
    return NULL;
}

// Monitor Site Status
void* monitor_site_status(void* arg) {
    while (!isCompleted) {
        sleep(10); // Simulate monitoring interval
        pthread_mutex_lock(&resource_mutex);
        cout << "---- Site Status ----\n";
        cout << "Bricks: " << bricks << ", Cement: " << cement << "\n";
        cout << "Workers:\n";
        for (const auto& worker : workers) {
            cout << worker.name << ": Energy = " << worker.energy
                 << ", Status = " << (worker.isWorking ? "Working" : "Idle") << "\n";
        }
        cout << "---------------------\n";
        pthread_mutex_unlock(&resource_mutex);
    }
    return NULL;
}

// Main Execution
int main() {
    initialize_resources();
    initialize_workers();
    initialize_tasks();

    vector<pthread_t> worker_threads;
    for (size_t i = 0; i < workers.size(); ++i) {
        pthread_t worker_thread;
        pthread_create(&worker_thread, NULL, simulate_worker_behavior, (void*)&workers[i]);
        worker_threads.push_back(worker_thread);
    }

    pthread_t resource_thread;
    pthread_create(&resource_thread, NULL, replenish_resources, NULL);

    pthread_t monitor_thread;
    pthread_create(&monitor_thread, NULL, monitor_site_status, NULL);

    // Wait for all threads to finish (manual termination required)
    for (size_t i = 0; i < worker_threads.size(); ++i) {
        pthread_join(worker_threads[i], NULL);
    }
    pthread_join(resource_thread, NULL);
    pthread_join(monitor_thread, NULL);

    //show remaning tasks and resources
    cout << "--------------All tasks are completed -----------------\n";
    cout << "All tasks completed? " << (high_priority_queue.empty() && high_priority_queue.empty() && high_priority_queue.empty())? "yes" : "no";
    cout << "\nBricks: " << bricks << ", Cement: " << cement << "\n";

    return 0;
}