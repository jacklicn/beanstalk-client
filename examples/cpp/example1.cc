#include "beanstalk.hpp"
#include <iostream>
#include <assert.h>

using namespace std;
using namespace Beanstalk;

int main() {
    Client client("127.0.0.1", 11300);
    assert(client.use("test"));
    assert(client.watch("test"));

    int id = client.put("hello");
    assert(id > 0);
    cout << "put job id: " << id << endl;

    Job job = client.reserve();
    assert(job.is_valid());
    assert(job.id() == id);

    cout << "reserved job id: "
         << job.id()
         << " with body {" << job.body() << "}"
         << endl;

    assert(client.del(job.id()));
    cout << "deleted job id: " << job.id() << endl;
}