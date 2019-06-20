#ifndef VAR_GET
#define VAR_GET(T, sym) \
public: std::add_const_t<T>& sym() const { return sym##_; } \
private: T sym##_;
#endif

#ifndef VAR_IO
#define VAR_IO(T, sym) \
public: std::add_const_t<T>& sym() const { return sym##_; } \
public: T& sym() { return sym##_; } \
public: void set_##sym(T&& obj) { sym##_ = std::move(obj); } \
private: T sym##_;
#endif

#ifndef VAR_PTR_GET
#define VAR_PTR_GET(T, sym) \
public: std::add_const_t<T>& sym() const { return *sym##_; } \
private: std::add_const_t<T>* sym##_;
#endif

#ifndef VAR_LOCAL_PTR_GET
#define VAR_LOCAL_PTR_GET(T, sym) \
public: std::add_const_t<T>& sym() const { return *sym##_; } \
public: T& sym() { return *sym##_; } \
private: T* sym##_;
#endif