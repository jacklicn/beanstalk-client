#include "beanstalk.hpp"
#include <stdexcept>
#include <sstream>
#include <iostream>

using namespace std;

namespace Beanstalk {

    Job::Job() {
        _id = 0;
    }

    Job::Job(int64_t id, const char *data, size_t size) {
        _body.assign(data, size);
        _id = id;
    }

    Job::Job(BSJ *job) {
        if (job) {
            _body.assign(job->data, job->size);
            _id = job->id;
            bs_free_job(job);
        }
        else {
            _id = 0;
        }
    }

    string& Job::body() {
        return _body;
    }

    int64_t Job::id() const {
        return _id;
    }

    /* start helpers */

    void parsedict(stringstream &stream, info_hash_t &dict) {
        string key, value;
        while(true) {
            stream >> key;
            if (stream.eof()) break;
            if (key[0] == '-') continue;
            stream >> value;
            key.erase(--key.end());
            dict[key] = value;
        }
    }

    void parselist(stringstream &stream, info_list_t &list) {
        string value;
        while(true) {
            stream >> value;
            if (stream.eof()) break;
            if (value[0] == '-') continue;
            list.push_back(value);
        }
    }

    /* end helpers */

    Client::~Client() {
        if (_handle > 0)
          bs_disconnect(_handle);
        _handle = -1;
    }

    Client::Client() {
        _handle       = -1;
        _host         = "";
        _port         = 0;
        _timeout_secs = 0;
    }

    Client::Client(const std::string& host, int port, float secs) {
        _handle       = -1;
        connect(host, port, secs);
    }

    void Client::connect(const std::string& host, int port, float secs) {
        if (_handle > 0)
            throw runtime_error("already connected to beanstalkd at " + _host);

        _host         = host;
        _port         = port;
        _timeout_secs = secs;

        _handle = secs > 0 ? bs_connect_with_timeout((char *)_host.c_str(), _port, secs) : bs_connect((char*)host.c_str(), port);
        if (_handle < 0)
            throw runtime_error("unable to connect to beanstalkd at " + _host);
    }

    bool Client::is_connected() {
      return _handle > 0;
    }

    bool Client::disconnect() {
        if (_handle > 0 && bs_disconnect(_handle) == BS_STATUS_OK) {
            _handle = -1;
            return true;
        }
        return false;
    }

    void Client::version(int *major, int *minor, int *patch) {
        bs_version(major, minor, patch);
    }

    void Client::reconnect() {
        disconnect();
        connect(_host, _port, _timeout_secs);
    }

    bool Client::use(const std::string& tube) {
        return bs_use(_handle, (const char*)tube.c_str()) == BS_STATUS_OK;
    }

    bool Client::watch(const std::string& tube) {
        return bs_watch(_handle, (const char*)tube.c_str()) == BS_STATUS_OK;
    }

    bool Client::ignore(const std::string& tube) {
        return bs_ignore(_handle, (const char*)tube.c_str()) == BS_STATUS_OK;
    }

    int64_t Client::put(const std::string& body, uint32_t priority, uint32_t delay, uint32_t ttr) {
        int64_t id = bs_put(_handle, priority, delay, ttr, (const char*)body.data(), body.size());
        return (id > 0 ? id : 0);
    }

    int64_t Client::put(const char *body, size_t bytes, uint32_t priority, uint32_t delay, uint32_t ttr) {
        int64_t id = bs_put(_handle, priority, delay, ttr, body, bytes);
        return (id > 0 ? id : 0);
    }

    bool Client::del(const Job &job) {
        return bs_delete(_handle, (int64_t)job.id()) == BS_STATUS_OK;
    }

    bool Client::del(int64_t id) {
        return bs_delete(_handle, id) == BS_STATUS_OK;
    }

    bool Client::reserve(Job &job) {
        BSJ *bsj;
        if (bs_reserve(_handle, &bsj) == BS_STATUS_OK) {
            job = bsj;
            return true;
        }
        return false;
    }

    bool Client::reserve(Job &job, uint32_t timeout) {
        BSJ *bsj;
        if (bs_reserve_with_timeout(_handle, timeout, &bsj) == BS_STATUS_OK) {
            job = bsj;
            return true;
        }
        return false;
    }

    bool Client::release(const Job &job, uint32_t priority, uint32_t delay) {
        return bs_release(_handle, (int64_t)job.id(), priority, delay) == BS_STATUS_OK;
    }

    bool Client::release(int64_t id, uint32_t priority, uint32_t delay) {
        return bs_release(_handle, id, priority, delay) == BS_STATUS_OK;
    }

    bool Client::bury(const Job &job, uint32_t priority) {
        return bs_bury(_handle, (int64_t)job.id(), priority) == BS_STATUS_OK;
    }

    bool Client::bury(int64_t id, uint32_t priority) {
        return bs_bury(_handle, id, priority) == BS_STATUS_OK;
    }

    bool Client::touch(const Job &job) {
        return bs_touch(_handle, (int64_t)job.id()) == BS_STATUS_OK;
    }

    bool Client::touch(int64_t id) {
        return bs_touch(_handle, id) == BS_STATUS_OK;
    }

    bool Client::peek(Job &job, int64_t id) {
        BSJ *bsj;
        if (bs_peek(_handle, id, &bsj) == BS_STATUS_OK) {
            job = bsj;
            return true;
        }
        return false;
    }

    bool Client::peek_ready(Job &job) {
        BSJ *bsj;
        if (bs_peek_ready(_handle, &bsj) == BS_STATUS_OK) {
            job = bsj;
            return true;
        }
        return false;
    }

    bool Client::peek_delayed(Job &job) {
        BSJ *bsj;
        if (bs_peek_delayed(_handle, &bsj) == BS_STATUS_OK) {
            job = bsj;
            return true;
        }
        return false;
    }

    bool Client::peek_buried(Job &job) {
        BSJ *bsj;
        if (bs_peek_buried(_handle, &bsj) == BS_STATUS_OK) {
            job = bsj;
            return true;
        }
        return false;
    }

    bool Client::kick(int bound) {
        return bs_kick(_handle, bound) == BS_STATUS_OK;
    }

    string Client::list_tube_used() {
        char *name;
        string tube;
        if (bs_list_tube_used(_handle, &name) == BS_STATUS_OK) {
            tube.assign(name);
            free(name);
        }

        return tube;
    }

    info_list_t Client::list_tubes() {
        char *yaml, *data;
        info_list_t tubes;
        if (bs_list_tubes(_handle, &yaml) == BS_STATUS_OK) {
            if ((data = strstr(yaml, "---"))) {
                stringstream stream(data);
                parselist(stream, tubes);
            }
            free(yaml);
        }
        return tubes;
    }

    info_list_t Client::list_tubes_watched() {
        char *yaml, *data;
        info_list_t tubes;
        if (bs_list_tubes_watched(_handle, &yaml) == BS_STATUS_OK) {
            if ((data = strstr(yaml, "---"))) {
                stringstream stream(data);
                parselist(stream, tubes);
            }
            free(yaml);
        }
        return tubes;
    }

    info_hash_t Client::stats() {
        char *yaml, *data;
        info_hash_t stats;
        string key, value;
        if (bs_stats(_handle, &yaml) == BS_STATUS_OK) {
            if ((data = strstr(yaml, "---"))) {
                stringstream stream(data);
                parsedict(stream, stats);
            }
            free(yaml);
        }
        return stats;
    }

    info_hash_t Client::stats_job(int64_t id) {
        char *yaml, *data;
        info_hash_t stats;
        string key, value;
        if (bs_stats_job(_handle, id, &yaml) == BS_STATUS_OK) {
            if ((data = strstr(yaml, "---"))) {
                stringstream stream(data);
                parsedict(stream, stats);
            }
            free(yaml);
        }
        return stats;
    }

    info_hash_t Client::stats_tube(const std::string& name) {
        char *yaml, *data;
        info_hash_t stats;
        string key, value;
        if (bs_stats_tube(_handle, (const char*)name.c_str(), &yaml) == BS_STATUS_OK) {
            if ((data = strstr(yaml, "---"))) {
                stringstream stream(data);
                parsedict(stream, stats);
            }
            free(yaml);
        }
        return stats;
    }

    bool Client::ping() {
        char *yaml;
        if (bs_list_tubes(_handle, &yaml) == BS_STATUS_OK) {
            free(yaml);
            return true;
        }

        return false;
    }
}
