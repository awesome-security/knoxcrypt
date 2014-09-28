#ifndef TEASAFE_UTILITY_CONCURRENT_QUEUE_HPP__
#define TEASAFE_UTILITY_CONCURRENT_QUEUE_HPP__

#include <queue>
#include <mutex>

//
// Note, this is not my code; found here:
// http://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html
// (c) Anthony Williams (assumed)
//

namespace teasafe {

    namespace utility {

        template<typename Data>
        class ConcurrentQueue
        {
        private:
            std::queue<Data> the_queue;
            mutable std::mutex the_mutex;
            std::condition_variable the_condition_variable;
            mutable bool m_wait;
        public:
            ConcurrentQueue()
              : m_wait(true)
            {
            }

            void stopWaiting(Data const& killSignal)
            {
                std::mutex::scoped_lock lock(the_mutex);
                the_queue.push(killSignal);
                m_wait = false;
                lock.unlock();
                the_condition_variable.notify_one();
            }

            void push(Data const& data)
            {
                std::mutex::scoped_lock lock(the_mutex);
                the_queue.push(data);
                lock.unlock();
                the_condition_variable.notify_one();
            }

            bool empty() const
            {
                std::mutex::scoped_lock lock(the_mutex);
                return the_queue.empty();
            }

            bool try_pop(Data& popped_value)
            {
                std::mutex::scoped_lock lock(the_mutex);
                if(the_queue.empty())
                {
                    return false;
                }

                popped_value=the_queue.front();
                the_queue.pop();
                return true;
            }

            void wait_and_pop(Data& popped_value)
            {
                std::mutex::scoped_lock lock(the_mutex);
                while(the_queue.empty() && m_wait)
                {
                    the_condition_variable.wait(lock);
                }

                popped_value=the_queue.front();
                the_queue.pop();
            }

        };

    }
}

#endif // TEASAFE_UTILITY_BLOCKING_QUEUE_HPP__
