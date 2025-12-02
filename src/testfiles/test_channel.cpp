#include <iostream>
#include <string>
#include <vector>
#include "Channel.hpp"
#include "Client.hpp"

// Mock send function
int send(int fd, const char* buf, size_t len, int flags)
{
    (void)flags;
    std::cout << "[MOCK SEND] fd: " << fd << ", msg: " << std::string(buf, len) << "\n";
    return static_cast<int>(len); // pretend send succeeded
}

int main()
{
    // Create clients
    Client alice(1);
    Client bob(2);
    Client charlie(3);

    alice.setNickname("Alice");
    bob.setNickname("Bob");
    charlie.setNickname("Charlie");

    // Create channel
    Channel channel;
    channel.setChannelName("#test");
    channel.setTopicName("Demo Channel");
    channel.setInviteMode(true);

    std::cout << "Channel: " << channel.getChannelName() << ", Topic: " << channel.getTopicName() << "\n";

    // Add members and operators
    channel.addMember(&alice);
    channel.addOperator(&bob);

    std::cout << "Members/Operators list: " << channel.getNamesList() << "\n";

    // Test promotion/demotion
    channel.setAsOperator("Alice"); // Promote Alice
    channel.setAsMember("Bob");     // Demote Bob
    std::cout << "After promotion/demotion: " << channel.getNamesList() << "\n";

    // Invite Charlie
    channel.addInvited("Charlie");
    std::cout << "Is Charlie invited? " << channel.isInvited("Charlie") << "\n";

    // Remove invite
    channel.removeInvited("Charlie");
    std::cout << "Is Charlie invited? " << channel.isInvited("Charlie") << "\n";

    // Test broadcast messages
    std::cout << "\n--- Testing broadcastMessage ---\n";
    channel.broadcastMessage("Hello everyone!");

    std::cout << "\n--- Testing broadcastMessageExcept (exclude Alice) ---\n";
    channel.broadcastMessageExcept("Hi everyone except Alice!", alice.getSocketFd());

    // Remove members/operators by fd
    channel.removeMemberByFd(alice.getSocketFd());
    channel.removeOperatorByFd(bob.getSocketFd());
    std::cout << "After removals: " << channel.getNamesList() << "\n";

    // Total clients
    std::cout << "Total clients in channel: " << channel.getChannelTotalClientCount() << "\n";

    return 0;
}
