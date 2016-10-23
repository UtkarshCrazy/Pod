#ifndef CODEC_FOR_IO_PARSING
#define CODEC_FOR_IO_PARSING
//Used to parse communication input 
//Used to create communication output
#include <boost/shared_ptr.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <string>
#include <atomic>
#include "events.hpp"
#include "sensor.h"

typedef boost::shared_ptr<user_select> user_select_ptr;

class codec{
    public:
        static user_select_ptr decode_input(const std::string & x);
        static void append_to_data_buffer(std::string & buff, const std::string & type, double *data, size_t data_length);
        static void append_to_data_buffer(std::string & buff, const std::string & type, std::atomic<double> const * const data, size_t data_length);
        static void create_message(sensor * sen, std::string & buff);
};
        

#endif
