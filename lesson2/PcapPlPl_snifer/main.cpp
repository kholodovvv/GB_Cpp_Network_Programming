#include <iostream>
#include <IPv4Layer.h>
#include <Packet.h>
#include <PcapFileDevice.h>
#include <PcapLiveDeviceList.h>
#include <SystemUtils.h>
#include <RawPacket.h>

static const std::string name_wr_file = "./pcpp_capture_packet";

bool WriteFile(pcpp::RawPacketVector &rawPacketVec){


    if(!pcpp::PcapFileWriterDevice(name_wr_file).open()){
        std::cerr << "COULD NOT OPEN THE FILE FOR WRITING!" << std::endl;
        return false;
    }

    if(!pcpp::PcapFileWriterDevice(name_wr_file).writePackets(rawPacketVec)){
        std::cerr << "FAILED TO WRITE DATA TO FILE!" << std::endl;
        pcpp::PcapFileWriterDevice(name_wr_file).close();
        return false;
    }

    pcpp::PcapFileWriterDevice(name_wr_file).close();

    return true;
}

bool OpenFile(std::string namefile){

    pcpp::PcapFileReaderDevice reader(namefile);
    if(!reader.open()){
        std::cerr << "Error opening Pcap file!";
        return false;
    }

    pcpp::RawPacket rawPacket;
    if(!reader.getNextPacket(rawPacket)){
        std::cerr << "Couldn't read the first packet in the file" << std::endl;
        return false;
    }

    // parse the raw packet into a parsed packet
    pcpp::Packet parsedPacket(&rawPacket);
    // verify the packet is IPv4
    if (parsedPacket.isPacketOfType(pcpp::IPv4))
    {
        // extract source and dest IPs
        pcpp::IPv4Address srcIP =
                parsedPacket.getLayerOfType<pcpp::IPv4Layer>()->getSrcIPv4Address();
        pcpp::IPv4Address destIP =
                parsedPacket.getLayerOfType<pcpp::IPv4Layer>()->getDstIPv4Address();
        // print source and dest IPs
        std::cout << "Source IP is '" << srcIP << "'; Dest IP is '" << destIP <<
                  "'" << std::endl;
    }

    reader.close();
}

pcpp::RawPacketVector pcappp_callback(pcpp::PcapLiveDevice &dev)
{
    pcpp::RawPacketVector rawPacketVec;

    dev.startCapture(rawPacketVec);
        pcpp::multiPlatformSleep(5);
    dev.stopCapture();

    return rawPacketVec;
}

static void PrintDeviceInfo(pcpp::PcapLiveDevice &lDevice){
    std::cout << "Network device info:" << std::endl;
    std::cout << "interface name: "
              << lDevice.getName() << "\n\n"
              << "Interface description: "
              << lDevice.getDesc()  << "\n\n"
              << "MAC address: "
              << lDevice.getMacAddress() << "\n\n"
              << "Default gateway: "
              << lDevice.getDefaultGateway() << std::endl;
}

int main() {

    std::cout << "1 - Open PCAP file, 2 - Capture packets, 3 - Exit" << std::endl;

    std::string choose_user_str;
    uint8_t choose_user_num;
    pcpp::PcapLiveDevice *findNetDevice;

    do{
        std::cin >> choose_user_str;
        choose_user_num = std::stoi(choose_user_str);
    }while(choose_user_num == 0 || choose_user_num > 3);

    if(choose_user_num == 1){
        std::cout << "ENTER THE PATH TO THE FILE:" << std::endl;
        std::cin >> choose_user_str;
        OpenFile(choose_user_str);

        std::cout << "PRESS ENTER..." << std::endl;
        std::cin >> choose_user_str;
        return EXIT_SUCCESS;
    }
    else if(choose_user_num == 3){
        return EXIT_SUCCESS;
    }

    std::cout << "SELECT A NETWORK DEVICE TO CAPTURE (1 - By Name, 2 - By Ip address):" << std::endl;

    do{
        std::cin >> choose_user_str;
        choose_user_num = std::stoi(choose_user_str);
    }while(choose_user_num == 0 || choose_user_num > 2);

    if(choose_user_num == 1) {
        std::cout << "ENTER THE NAME OF THE NETWORK ADAPTER:" << std::endl;

        do {
            std::cin >> choose_user_str;
        } while (choose_user_str.length() < 2);

        findNetDevice = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName(
                choose_user_str);
        if (findNetDevice == NULL) {
            std::cerr << "Network device with name: '" << choose_user_str << "' not found!" << std::endl;
            return EXIT_FAILURE;
        } else {
            PrintDeviceInfo(*findNetDevice);
        }
    }else{
        std::cout << "ENTER THE IP ADDRESS OF THE NETWORK ADAPTER:" << std::endl;

        do{
            std::cin >> choose_user_str;
        }while(choose_user_str.length() < 6);

        findNetDevice = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByIp(choose_user_str);
        if(findNetDevice == NULL) {
            std::cerr << "Network device with address: " << choose_user_str << " not found!" << std::endl;
            return EXIT_FAILURE;
        }else {
            PrintDeviceInfo(*findNetDevice);
        }
    }

    //open the device before start capturing
    if(!findNetDevice->open()){
        std::cerr << "Cannot open device!" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Starting capture ..." << std::endl;

    pcpp::RawPacketVector rawPacketVec = pcappp_callback(*findNetDevice);

    std::vector<pcpp::Packet> vecParsePacket;
    std::vector<pcpp::Packet>::iterator it;

    //Вывод основных сведений со всех слоев пакета на экран
    pcpp::RawPacketVector::ConstVectorIterator iter;
    for(iter = rawPacketVec.begin(); iter != rawPacketVec.end(); iter++){
        vecParsePacket.push_back(pcpp::Packet(*iter, false, pcpp::LINKTYPE_ETHERNET));
    }

    for(it = vecParsePacket.begin(); it != vecParsePacket.end(); it++){
        std::cout << it->toString() << std::endl;
    }

    std::cout << "SAVE TO FILE? (Y/N)" << std::endl;
    do{
        std::cin >> choose_user_str;
    }while(choose_user_str.length() == 0 || choose_user_str.length() > 1);

    if(choose_user_str == "Y" || choose_user_str == "y"){
        WriteFile(rawPacketVec);
    }

    std::cout << "Sending packets ..." << std::endl;

    //Отправка пакетов обратно в сеть
    for(iter = rawPacketVec.begin(); iter != rawPacketVec.end(); iter++){
        if(!findNetDevice->sendPacket(**iter)){
            std::cerr << "Couldn't send packet" << std::endl;
        }

        std::cout << rawPacketVec.size() << " packets sent." << std::endl;
    }


    std::cout << "Closing capture ..." << std::endl;
    findNetDevice->close();

    return 0;
}
