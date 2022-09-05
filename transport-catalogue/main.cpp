#include "serialization.h"

#include "json_reader.h"
#include "request_handler.h"
#include "transport_router.h"

using namespace std;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"s;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    const std::string mode(argv[1]);

    if (mode == "make_base"s) {

        Serializator ser(std::cin);
        ser.Pack();

    } else if (mode == "process_requests"s) {

        DeSerializator de_ser(std::cin);
        json::Document base_request = de_ser.UnPack();
        
        const json::Document& process_request = de_ser.GetProcessRequests();
        std::cerr << "read is OK"s << std::endl;
        // создаем транспортный справочник
        transport_catalogue::TransportCatalogue data;
        map_renderer::MapRenderer mr;
        json_reader::jsonReader jr_1(base_request, data, mr);
        // создаем каталог маршрутов
        transport_router::TransportRouter rt(data);
        rt.BuildGraph();
        // обрабатываем запросы к базе
        request_handler::RequestHandler rh(data, mr, rt);
        json_reader::jsonReader jr_2(process_request, rh);
        json::Document out = rh.DatabaseOutput();
        json::Print(out, std::cout);
 
    } else {
        PrintUsage();
        return 1;
    }
    return 0;
}

// run process_requests < ../input/process_requests_1.txt > ../output/out_1.txt
