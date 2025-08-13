#pragma once

#define BOOST_LOG_NO_WCHAR_T
#define BOOST_LOG_WITHOUT_WCHAR_T

#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/json.hpp>
#include <boost/log/attributes/value_extraction.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

namespace logger {
    namespace logging = boost::log;
    namespace sinks = boost::log::sinks;
    namespace json = boost::json;
    namespace keywords = boost::log::keywords;

    BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)

        class JsonFormatter {
        public:
            void operator()(logging::record_view const& rec, logging::formatting_ostream& strm) const {
                json::object obj;


                auto ts = logging::extract<boost::posix_time::ptime>("TimeStamp", rec);
                if (ts) {
                    obj["timestamp"] = boost::posix_time::to_iso_extended_string(ts.get());
                }


                auto msg = logging::extract<std::string>("Message", rec);
                if (msg) {
                    obj["message"] = msg.get();
                }


                if (auto data = rec[additional_data]; data) {
                    obj["data"] = data.get();
                }

                strm << json::serialize(obj);
            }
    };

    inline void InitLogging() {
        logging::add_common_attributes();

        auto core = logging::core::get();
        core->remove_all_sinks();


        auto sink = boost::make_shared<sinks::synchronous_sink<sinks::text_ostream_backend>>();

        boost::shared_ptr<std::ostream> stream(&std::clog, boost::null_deleter());
        sink->locked_backend()->add_stream(stream);

        sink->set_formatter(JsonFormatter());
        sink->locked_backend()->auto_flush(true);

        core->add_sink(sink);
    }
} // namespace logger