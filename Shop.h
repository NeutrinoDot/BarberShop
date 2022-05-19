// Matthew Kim CSS503 A
// Modified: 08/05/22
// --------------------------------------------------------------
// Defines the methods for the barber and customer within the barber shop.
// Barbers can accept to provide service or send off customer after providing
// service. 
// --------------------------------------------------------------

#ifndef Shop_H_
#define Shop_H_
#include <pthread.h>
#include <iostream>
#include <sstream>
#include <string>
#include <queue>
using namespace std;

#define kDefaultNumChairs 3
#define kDfaultBarbers 1

class Shop {
public:
  Shop() : num_barbers(kDfaultBarbers), num_chairs(kDefaultNumChairs) {
    init();
  };

  Shop(int num_barbers, int num_chairs) : 
    num_barbers((num_barbers > 0) ? num_barbers : kDefaultNumChairs), 
    num_chairs((num_chairs >= 0) ? num_chairs : kDefaultNumChairs) {
    init();
  };

  ~Shop();
  
  int visitShop(int custId);   // returns barber ID or -1 (not served)
  void leaveShop(int custId, int barberId);
  void helloCustomer(int barberId);
  void byeCustomer(int barberId);
  int get_cust_drops() const;

private:
  const int num_barbers; // total number of barbers
  const int num_chairs; // the max number of customer threads that can wait
  int cust_drops; // total count of dropped customers

  int* customer_in_chair; // customer id in the barber chairs
  int in_chair_count;

  bool* in_service; // whether a barber is currently providing service 
  int wait_count; // counter for waiting customers

  bool* money_paid;

  // Mutexes and condition variables to coordinate threads
  // mMutex is used in conjuction with all conditional variables
  pthread_mutex_t mMutex;

  // visitShop customers wait. byeCustomer barber signals
  pthread_cond_t  cond_customers_waiting; 

  // leaveShop customer waits for barber to finish. byeCustomer barber signals.
  pthread_cond_t* cond_customer_served; 

  // leaveShop customer signal barber. byeCustomer barber waits.
  pthread_cond_t* cond_barber_paid; 

  // visitShop customer signal a unique barber. helloCustomer barber waits.
  pthread_cond_t* cond_barber_sleeping; 
  
  void init();
  string int2string(int i);
  void print(bool isCustomer, int person, string message);
};
#endif
