#ifndef SINGLETON_H
#define SINGLETON_H

template <typename T>
class SingleTon
{
private:
    inline SingleTon() {}
    SingleTon(const SingleTon&)=delete;
    SingleTon(SingleTon&&)=delete;
    SingleTon& operator=(const SingleTon&)=delete;
    SingleTon& operator=(SingleTon&&)=delete;

    T instance;

public:
    static T* getInstance(){
        static SingleTon* instance;
        if(instance == nullptr) instance = new SingleTon;
        return &(instance->instance);
    }

    static inline T& getReference(){ return *getInstance(); }
};

#endif // SINGLETON_H
