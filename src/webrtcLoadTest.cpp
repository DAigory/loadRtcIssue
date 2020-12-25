// webrtcLoadTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "streams.h"

void callable()
{
    std::cout << "callable";
    auto streams = new Streams();
    streams->Connect();
}

int main()
{
    
    std::thread t1(callable);
    while (true)
    {

    }
  //  auto test = new Test();
  //  streams->CreateFactory();
   // streams->Connect();
}
