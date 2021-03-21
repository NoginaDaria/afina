#include <afina/concurrency/Executor.h>
#include <iostream>

namespace Afina {
namespace Concurrency {
    Executor::Executor(size_t low_watermark, size_t high_watermark, size_t max_queue_size, std::chrono::milliseconds idle_time)
        : low_watermark(low_watermark), high_watermark(high_watermark), max_queue_size(max_queue_size), idle_time(idle_time)	    
    {
        running_threads = 0;
	state = State::kRun;
	for (size_t i=0; i < low_watermark; i++){
	    threads.emplace_back(std::thread(&Executor::perform, this));
	}
    }

    Executor::~Executor(){
        Stop(true);
    }

    void Executor::Stop(bool await){
        std::unique_lock<std::mutex> lock(mutex);
	if (state == State::kStopped){
	   return;
	}
	state = State::kStopping;
	empty_condition.notify_all();
	
	while (!threads.empty() && await){
	    stop_condition.wait(lock);
	}
	state = State::kStopped;
    }

    void Executor::perform(){
        std::unique_lock<std::mutex> lock(mutex);
	while (state == State::kRun){
            
	    if (tasks.empty()){
                auto curr_time = std::chrono::system_clock::now();
		auto waiting = empty_condition.wait_until(lock, curr_time + idle_time);
		if ((waiting == std::cv_status::timeout) && (threads.size() > low_watermark)){
		    break;
		} else {
		    continue;
		}
	    }
	    
	    auto task = std::move(tasks.front());
	    tasks.pop_front();
	    running_threads++;

	    lock.unlock();
	    try {task();}
	    catch(std::exception &ex){
                std::cerr << "Error while performing a task: " << ex.what() << std::endl;
	    }
	    lock.lock();
	    
	    running_threads--;
	}

        auto curr_thread_id = std::this_thread::get_id();
	for (auto it = threads.begin(); it < threads.end(); ++it){
	    if (it->get_id() == curr_thread_id){
		auto curr_thread = std::move(*it);
                threads.erase(it);
		curr_thread.detach();
		if (threads.empty()){
		    stop_condition.notify_all();
		}
		break;
	    }
	}
    }

} // namespace Concurrency
} // namespace Afina
