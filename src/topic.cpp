#include "Server.hpp"

static const size_t MAX_TOPIC_LENGTH = 332;

static std::string trim(const std::string &str)
{
    size_t start = 0;
    while (start < str.size() && std::isspace(str[start])) ++start;
    size_t end = str.size();
    while (end > start && std::isspace(str[end - 1])) --end;
    return str.substr(start, end - start);
}

static bool isValidTopic(const std::string &topic)
{
    for (size_t i = 0; i < topic.size(); ++i)
    {
        unsigned char c = topic[i];
        if (c <= 31 || c == 127)
            return false;
    }
    return true;
}

void Server::TOPIC(const std::string &cmd, int fd)
{
    Client *client = getClientByFd(fd);
    if (!client) return;

    std::vector<std::string> tokens = splitCommand(cmd);
    std::string nickname = client->getNickname();

    if (tokens.size() < 2)
    {
        sendResponse(fd, "461 " + nickname + " TOPIC :Not enough parameters\r\n");
        return;
    }
    std::string channelName = tokens[1];
    if (!isValidChannelName(channelName))
    {
        sendResponse(fd, "403 " + nickname + " " + channelName + " :No such channel\r\n");
        return;
    }
    std::string internalChannelName = channelName.substr(1);
    Channel *ch = getChannel(internalChannelName);
    if (!ch)
    {
        sendResponse(fd, "403 " + nickname + " " + channelName + " :No such channel\r\n");
        return;
    }
    if (!ch->isInChannel(nickname))
    {
        sendResponse(fd, "442 " + nickname + " " + channelName + " :You're not on that channel\r\n");
        return;
    }
    // QUERY current topic if no new topic parameter
    if (tokens.size() == 2)
    {
        std::string currentTopic = ch->getTopicName();
        if (currentTopic.empty())
        {
            sendResponse(fd, "331 " + nickname + " " + channelName + " :No topic is set\r\n");
        }
        else
        {
            sendResponse(fd, "332 " + nickname + " " + channelName + " :" + currentTopic + "\r\n");
            sendResponse(fd, "333 " + nickname + " " + channelName + " " + ch->getTopicSetter() + " " + ch->getTimeTopicCreated() + "\r\n");
        }
        return;
    }

    if (tokens.size() > 3)
    {
        sendResponse(fd, "461 " + nickname + " TOPIC :Too many parameters\r\n");
        return;
    }
    // Set or clear topic
    std::string newTopic = tokens[2];
    if (!newTopic.empty() && newTopic[0] == ':')
        newTopic = newTopic.substr(1);
    newTopic = trim(newTopic);
    // Validate topic
    if (!isValidTopic(newTopic))
    {
        sendResponse(fd, "461 " + nickname + " TOPIC :Invalid characters in topic\r\n");
        return;
    }
    if (newTopic.size() > MAX_TOPIC_LENGTH)
    {
        sendResponse(fd, "461 " + nickname + " TOPIC :Topic is too long\r\n");
        return;
    }
    // Check operator privileges if topic is protected
    if (ch->getTopicMode() && !ch->isChannelOperator(nickname))
    {
        sendResponse(fd, "482 " + nickname + " " + channelName + " :You're not channel operator\r\n");
        return;
    }
    // Set topic and record setter/time
    ch->setTopicName(newTopic);
    ch->setTopicSetter(nickname);
    ch->setTimeTopicCreated();
    // Broadcast topic change
    std::ostringstream oss;
    oss << ":" << client->getPrefix() << " TOPIC " << channelName << " :";
    if (!newTopic.empty())
        oss << newTopic;
    oss << "\r\n";
    ch->broadcastMessage(oss.str());
}
