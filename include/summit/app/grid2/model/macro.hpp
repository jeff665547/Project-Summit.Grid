#ifndef VAR_GET
#define VAR_GET(T, sym) \
public: std::add_const_t<T>& sym() const { return sym##_; } \
private: T sym##_;
#endif
#ifndef VAR_PTR_GET
#define VAR_PTR_GET(T, sym) \
public: std::add_const_t<T>& sym() const { return *sym##_; } \
private: std::add_const_t<T>* sym##_;
#endif