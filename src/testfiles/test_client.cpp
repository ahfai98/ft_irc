#include "Server.hpp"
#include "Client.hpp"

int main()
{
    // Create a Client with socket fd 42
    Client client(42);

    // Test getters
    std::cout << "Socket FD: " << client.getSocketFd() << std::endl;
    std::cout << "Username: " << client.getUsername() << std::endl;
    std::cout << "Nickname: " << client.getNickname() << std::endl;

    // Test setters
    client.setUsername("alice_user");
    client.setNickname("Alice");
    client.setPasswordAuthenticated(true);
    client.setFullyAuthenticated(true);
    client.setIpAddress("127.0.0.1");
    client.setBuffer("Hello IRC");

    std::cout << "After setting:" << std::endl;
    std::cout << "Username: " << client.getUsername() << std::endl;
    std::cout << "Nickname: " << client.getNickname() << std::endl;
    std::cout << "Authenticated: " 
              << client.getPasswordAuthenticated() << " / " 
              << client.getFullyAuthenticated() << std::endl;
    std::cout << "Buffer: " << client.getBuffer() << std::endl;

    // Test clearing buffer
    client.clearBuffer();
    std::cout << "Buffer after clear: '" << client.getBuffer() << "'" << std::endl;

    // Test channel invites
    client.addChannelInvites("#channel1");
    client.addChannelInvites("#channel2");

    // Test joined channels
    client.addJoinedChannels("#general");
    client.addJoinedChannels("#random");

    std::cout << "Is in #general? " << client.isInChannel("#general") << std::endl;
    client.removeJoinedChannels("#general");
    std::cout << "Is in #general after removal? " << client.isInChannel("#general") << std::endl;

    // List all joined channels
    const std::vector<std::string>& joined = client.getJoinedChannels();
    std::cout << "Joined channels:" << std::endl;
    for (std::vector<std::string>::const_iterator it = joined.begin(); it != joined.end(); ++it)
    {
        std::cout << "  " << *it << std::endl;
    }

    return 0;
}
