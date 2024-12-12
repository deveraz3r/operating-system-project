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
bool isCompleted = false; // Notify all processes of task completion

// Mutex for resource synchronization
pthread_mutex_t brick_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cement_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t wheel_cart_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t task_mutex = PTHREAD_MUTEX_INITIALIZER;

enum class Priority {
    HIGH,
    MEDIUM,
    LOW,
};

// Task structure
struct Task {
    string name;
    Priority priority;
    int bricksRequired;
    int cementRequired;
    int timeRequired;
};

// Task Queues
queue<Task> high_priority_queue;
queue<Task> medium_priority_queue;
queue<Task> low_priority_queue;

// Skill enum
enum class Skill {
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

// Helper Functions
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
    // // Hardcoded tasks for High Priority
    // high_priority_queue.push({"Build Foundation", Priority::HIGH, 20, 10, 24});
    // high_priority_queue.push({"Construct Roof", Priority::HIGH, 15, 10, 18});

    // // Hardcoded tasks for Medium Priority
    // medium_priority_queue.push({"Install Windows", Priority::MEDIUM, 10, 8, 12});
    // medium_priority_queue.push({"Paint Walls", Priority::MEDIUM, 0, 14, 8});

    // // Hardcoded tasks for Low Priority
    // low_priority_queue.push({"Landscaping", Priority::LOW, 5, 3, 10});
    // low_priority_queue.push({"Clean Site", Priority::LOW, 0, 10, 4});

    int numTasks;
    cout << "Enter the number of tasks: ";
    cin >> numTasks;

    for (int i = 0; i < numTasks; ++i) {
        string name;
        int priority, bricksRequired, cementRequired, timeRequired;

        cout << "Enter details for Task " << i + 1 << ":\n";
        cout << "Task Name: ";
        cin.ignore(); // Clear input buffer
        getline(cin, name); // Read the full task name

        cout << "Priority (0 = HIGH, 1 = MEDIUM, 2 = LOW): ";
        cin >> priority;  // Here, we directly read the integer priority

        // Convert integer priority to enum value
        Priority taskPriority;
        switch (priority) {
            case 0:
                taskPriority = Priority::HIGH;
                break;
            case 1:
                taskPriority = Priority::MEDIUM;
                break;
            case 2:
                taskPriority = Priority::LOW;
                break;
            default:
                cout << "Invalid priority input. Assigning default priority: HIGH.\n";
                taskPriority = Priority::HIGH;  // Default to HIGH if invalid input
                break;
        }

        cout << "Bricks Required: ";
        cin >> bricksRequired;

        cout << "Cement Required: ";
        cin >> cementRequired;

        cout << "Time Required (hours): ";
        cin >> timeRequired;

        // Create the Task object with the mapped priority
        Task newTask{name, taskPriority, bricksRequired, cementRequired, timeRequired};

        // Add the task to the appropriate queue based on the priority
        switch (taskPriority) {
            case Priority::HIGH:
                high_priority_queue.push(newTask);
                break;
            case Priority::MEDIUM:
                medium_priority_queue.push(newTask);
                break;
            case Priority::LOW:
                low_priority_queue.push(newTask);
                break;
        }
    }

    cout << "Tasks initialized.\n";
}


// Task Scheduling with Task Mutex
Task assign_task(Worker* worker) {
    pthread_mutex_lock(&task_mutex); // Lock task mutex

    if (!high_priority_queue.empty() && worker->skill == Skill::HIGH) {
        Task task = high_priority_queue.front();
        high_priority_queue.pop();
        pthread_mutex_unlock(&task_mutex);
        return task;
    }
    if (!medium_priority_queue.empty() && (worker->skill == Skill::MEDIUM || worker->skill == Skill::HIGH)) {
        Task task = medium_priority_queue.front();
        medium_priority_queue.pop();
        pthread_mutex_unlock(&task_mutex);
        return task;
    }
    if (!low_priority_queue.empty() && (worker->skill == Skill::LOW || worker->skill == Skill::MEDIUM || worker->skill == Skill::HIGH)) {
        Task task = low_priority_queue.front();
        low_priority_queue.pop();
        pthread_mutex_unlock(&task_mutex);
        return task;
    }

    bool isWorking = false;
    for (size_t i = 0; i < workers.size(); ++i) {
        if(workers[i].isWorking){
            isWorking = true;
        }
    }
    
    if(!isWorking){
        isCompleted = true; // All tasks are completed
    }
    pthread_mutex_unlock(&task_mutex);
    return {"", Priority::LOW, 0, 0, 0}; // Return an empty task
}

// Resource Management
bool allocate_bricks() {
    pthread_mutex_lock(&brick_mutex);

    // Check if bricks are available
    if (bricks > 0) { // Corrected condition
        // Use the wheel cart (synchronized with its mutex)
        pthread_mutex_lock(&wheel_cart_mutex);
        cout << "Using wheel_cart to transfer 1 unit of bricks.\n";

        --bricks;

        cout << "Allocated 1 unit of bricks. Remaining: " << bricks << ".\n";
        pthread_mutex_unlock(&wheel_cart_mutex);
        pthread_mutex_unlock(&brick_mutex);
        return true;
    }

    pthread_mutex_unlock(&brick_mutex);
    return false;
}

bool allocate_cement() {
    pthread_mutex_lock(&cement_mutex);

    // Check if cement is available
    if (cement > 0) {
        // Use the wheel cart (synchronized with its mutex)
        pthread_mutex_lock(&wheel_cart_mutex);
        cout << "Using wheel_cart to transfer 1 unit of cement.\n";

        --cement;

        cout << "Allocated 1 unit of cement. Remaining: " << cement << ".\n";
        pthread_mutex_unlock(&wheel_cart_mutex);
        pthread_mutex_unlock(&cement_mutex);
        return true;
    }

    pthread_mutex_unlock(&cement_mutex);
    return false;
}

void* regenerate_resources(void*) {
    while (!isCompleted) {
        sleep(5); // Simulate replenishment delay
        pthread_mutex_lock(&brick_mutex);
        pthread_mutex_lock(&cement_mutex);
        bricks += 5; // Add 5 units of bricks
        cement += 5; // Add 5 units of cement
        cout << "Resources replenished: Bricks = " << bricks << ", Cement = " << cement << ".\n";
        pthread_mutex_unlock(&brick_mutex);
        pthread_mutex_unlock(&cement_mutex);
    }
    return NULL;
}

struct SimulateThreadArgs {
    Worker worker;
    Task task;
};

void* simulate_task(void* arg) {
    SimulateThreadArgs* args = static_cast<SimulateThreadArgs*>(arg);
    Worker* worker = &args->worker;
    Task task = args->task;

    // Allocate required resources
    while (task.bricksRequired > 0 || task.cementRequired > 0) {
        bool bricksAllocated = false, cementAllocated = false;

        if (task.bricksRequired > 0) {
            bricksAllocated = allocate_bricks();
            if (bricksAllocated) --task.bricksRequired;
        }

        if (task.cementRequired > 0) {
            cementAllocated = allocate_cement();
            if (cementAllocated) --task.cementRequired;
        }

        if (!bricksAllocated && task.bricksRequired > 0) {
            cout << worker->name << " waiting for bricks.\n";
            sleep(1); // Wait for resources
        }

        if (!cementAllocated && task.cementRequired > 0) {
            cout << worker->name << " waiting for cement.\n";
            sleep(1); // Wait for resources
        }
    }

    // Simulate task completion
    cout << worker->name << " started task: " << task.name << "\n";
    sleep(task.timeRequired);

    // Deduct worker energy
    worker->energy -= 20; // energy deduction
    if (worker->energy < 0) worker->energy = 0; // Ensure energy doesn't go negative

    cout << worker->name << " completed task: " << task.name << "\n";
    worker->isWorking = false;

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

        Task task = assign_task(worker);
        if (task.name.empty()) { // Check for an empty task
            worker->isWorking = false;
            sleep(1); // Wait for new tasks
            continue;
        }

        worker->isWorking = true;
        pthread_t task_thread;
        SimulateThreadArgs args = { *worker, task };
        pthread_create(&task_thread, NULL, simulate_task, (void*)&args);

        pthread_join(task_thread, NULL); // Wait for the task to complete
    }
    return NULL;
}

// Monitor Site Status
void* monitor_site_status(void* arg) {
    while (!isCompleted) {
        sleep(10); // Simulate monitoring interval
        pthread_mutex_unlock(&brick_mutex);
        pthread_mutex_unlock(&cement_mutex);
        cout << "---- Site Status ----\n";
        cout << "Bricks: " << bricks << ", Cement: " << cement << "\n";
        cout << "Workers:\n";
        for (const auto& worker : workers) {
            cout << worker.name << ": Energy = " << worker.energy
                 << ", Status = " << (worker.isWorking ? "Working" : "Idle") << "\n";
        }
        cout << "---------------------\n";
        pthread_mutex_unlock(&brick_mutex);
        pthread_mutex_unlock(&cement_mutex);
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
    pthread_create(&resource_thread, NULL, regenerate_resources, NULL);

    pthread_t monitor_thread;
    pthread_create(&monitor_thread, NULL, monitor_site_status, NULL);

    // Wait for all threads to finish (manual termination required)
    for (size_t i = 0; i < worker_threads.size(); ++i) {
        pthread_join(worker_threads[i], NULL);
    }
    pthread_join(resource_thread, NULL);
    pthread_join(monitor_thread, NULL);

    // Show remaining tasks and resources
    cout << "-------------- All tasks are completed -----------------\n";
    cout << "All tasks completed? " << (high_priority_queue.empty() && medium_priority_queue.empty() && low_priority_queue.empty() ? "Yes" : "No");
    cout << "\nBricks: " << bricks << ", Cement: " << cement << "\n";

    return 0;
}