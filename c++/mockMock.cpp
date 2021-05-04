#include <iostream>
#include <chrono>
#include <thread>

using namespace std;

struct Entity
{
  int Age;
  int Weight;
  const char *name;
};

static const int NUM_CALLS = 3;

template <typename T>
class Calls
{
public:
  struct Call
  {
    unsigned long time;
    Entity entity;
  };

  Call calls[NUM_CALLS];

public:
  void store(int idx, T entity)
  {
    // cout << "IN:   Age: " << entity.Age << " Weight: " << entity.Weight << " Name: " << entity.name << std::endl;
    calls[idx].entity = entity;

    // cout << "READ: Age: " << calls[idx].entity.Age << " Weight: " << calls[idx].entity.Weight << " Name: " << calls[idx].entity.name << std::endl;
  }

  T read(int offset)
  {
    return calls[offset].entity;
  }
};

static Calls<Entity> calls;

static void mockMock(int i, Entity e)
{
  calls.store(i, e);
}

int main()
{

  cout << "running" << std::endl;

  {
    Entity liam;
    liam.Age = 6;
    liam.Weight = 14;
    liam.name = "Liam";

    Entity leila;
    leila.Age = 45;
    leila.Weight = 45;
    leila.name = "Leila";

    Entity sean;
    sean.Age = 49;
    sean.Weight = 70;
    sean.name = "Sean";

    mockMock(0, liam);

    mockMock(1, leila);

    mockMock(2, sean);
  }
  Entity out;

  out = calls.read(0);
  cout << "OUT: Age: " << out.Age << " Weight: " << out.Weight << " Name: " << out.name << std::endl;

  out = calls.read(1);
  cout << "OUT: Age: " << out.Age << " Weight: " << out.Weight << " Name: " << out.name << std::endl;

  out = calls.read(2);
  cout << "OUT: Age: " << out.Age << " Weight: " << out.Weight << " Name: " << out.name << std::endl;

  std::this_thread::sleep_for(std::chrono::seconds(1));
  std::this_thread::sleep_for(std::chrono::seconds(1));

  cout << "Finished" << std::endl;
  return 0;
}