#include "serialization.h"

#include "json_reader.h"
#include "request_handler.h"
#include "transport_router.h"

#include <optional>

using namespace std;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"s;
}

optional<transport_catalogue_proto::TransportCatalogueProto> Deserialize(const filesystem::path& path) {
    ifstream in_file(path, ios::binary);
    transport_catalogue_proto::TransportCatalogueProto proto;
    if (!proto.ParseFromIstream(&in_file)) {
        return nullopt;
    }

    // тут нужен move, поскольку возвращается другой тип
    return {move(proto)};
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    const std::string mode(argv[1]);

    if (mode == "make_base"s) {
        json::Document doc = json::Load(std::cin);
        // создаем транспортный справочник
        transport_catalogue::TransportCatalogue data;
        map_renderer::MapRenderer mr;
        json_reader::jsonReader jr_1(doc, data, mr);

        // сохраняем в файл
        const std::filesystem::path path = doc.GetRoot().AsMap().at("serialization_settings").AsMap().at("file").AsString();
        std::ofstream out_file(path, std::ios::binary);
        MakeBase make_base(data, mr);
        transport_catalogue_proto::TransportCatalogueProto proto = make_base.Pack();
        proto.SerializeToOstream(&out_file);

    } else if (mode == "process_requests"s) {
        json::Document doc = json::Load(std::cin);
        const std::filesystem::path path = doc.GetRoot().AsMap().at("serialization_settings").AsMap().at("file").AsString();
        // распаковываем файл
        optional<transport_catalogue_proto::TransportCatalogueProto> proto = Deserialize(path);

        transport_catalogue::TransportCatalogue data;
        map_renderer::MapRenderer mr;
        ProcessRequests process_requests(proto.value(), data, mr);
        process_requests.UnPack();
        
        // создаем каталог маршрутов
        transport_router::TransportRouter rt(data);
        rt.BuildGraph();
        // обрабатываем запросы к базе
        request_handler::RequestHandler rh(data, mr, rt);
        json_reader::jsonReader jr(doc, rh);

        json::Document out = rh.DatabaseOutput();
        json::Print(out, std::cout);
        return 0;
    } else {
        PrintUsage();
        return 1;
    }
    return 0;
}

// run process_requests < ../input/process_requests_1.txt > ../output/out_1.txt
