template<class T>
struct SlidingWindowLog {
	void SetCapacity(size_t tgt);
	
	void SetTimeWidth(double cnt_sec);
};

template<class T>
struct TokenBucket {
	void SetCapacity(size_t tgt);
	
	void SetRefillRate(double tk_per_sec);
};

template<tepmlate<typename> typename Feature>
struct IRateLimiter : Feature<IRateLimiter> {
	bool SendRequest(const key_type& key); // What is key_type ?
};
