/*

BSD 3-Clause License

Copyright (c) 2021, Mason Hieb
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#pragma once
#include<mutex>
#include<queue>
#include<condition_variable>

// A thread safe queue that blocks and returns a value on "pop()"
template <typename T>
class thread_safe_queue {
public:
	std::atomic_bool empty() {
		std::lock_guard<std::mutex> lk(m_);
		return q_.empty();
	}
	T pop() {
		std::unique_lock<std::mutex> lk(m_);
		cv_.wait(lk, [this]() {return !q_.empty(); });
		T out = q_.front();
		q_.pop();
		return out;
	}
	template <class... valty>
	void emplace(valty&&... val) {
		std::lock_guard<std::mutex> lk(m_);
		q_.emplace(std::forward<valty>(val)...);
		notify();
	}
	void push(T val) {
		std::lock_guard<std::mutex> lk(m_);
		q_.push(val);
		notify();
	}
	void notify() {
		cv_.notify_all();
	}
private:
	std::queue<T> q_;
	std::mutex m_;
	std::condition_variable cv_;
};