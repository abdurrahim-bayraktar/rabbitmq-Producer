#include <rmqa_connectionstring.h>
#include <rmqa_producer.h>
#include <rmqa_rabbitcontext.h>
#include <rmqa_topology.h>
#include <rmqt_exchange.h>
#include <rmqt_message.h>
#include <rmqt_result.h>


#include <iostream>
#include <string>

using namespace BloombergLP;

int main()
{

    const char* AMQP_URI = "amqp://hello:hello@localhost:5672/hello-vhost";



    rmqa::RabbitContext rabbit;


    bsl::optional<rmqt::VHostInfo> vhostInfo = rmqa::ConnectionString::parse(AMQP_URI);
    if (!vhostInfo) {
        std::cerr << "Bad AMQP URI\n";
        return 1;
    }


    // returns quickly; actual connect happens asynchronously
    bsl::shared_ptr<rmqa::VHost> vhost = rabbit.createVHostConnection("simple-producer",
    vhostInfo.value());


    // declare a tiny topology: exchange + queue + binding
    rmqa::Topology topology;
    rmqt::ExchangeHandle exchange = topology.addExchange("hello-exchange");
    rmqt::QueueHandle queue = topology.addQueue("hello-queue");

    topology.bind(exchange, queue, "hello");


    const uint16_t maxOutstandingConfirms = 10;


    rmqt::Result<rmqa::Producer> prodRes =
    vhost->createProducer(topology, exchange, maxOutstandingConfirms);


    if (!prodRes) {
        std::cerr << "Failed creating producer: " << prodRes.error() << "\n";
        return 1;
    }


    bsl::shared_ptr<rmqa::Producer> producer = prodRes.value();


    // the message body
    std::string body = "Hello, world!";
    rmqt::Message msg(
    bsl::make_shared<bsl::vector<uint8_t> >(body.cbegin(), body.cend()));


    auto status = producer->send(
    msg,
    "hello", // routing key
    [](const rmqt::Message& message,
    const bsl::string& routingKey,
    const rmqt::ConfirmResponse& response) {
    if (response.status() == rmqt::ConfirmResponse::ACK) {
    std::cout << "Message confirmed: " << message.guid() << "\n";
    }
    else {
    std::cerr << "Message NOT confirmed: " << message.guid() << " "
    << response << "\n";
    }
    });


    if (status != rmqp::Producer::SENDING) {
        std::cerr << "Send failed (status != SENDING)\n";
        return 1;
    }


    // wait for the broker to confirm (so program doesn't exit immediately)
    producer->waitForConfirms();


    return 0;
}