#include "BoxControllerWebDataServer.h"
#include <Wt/WServer.h>
#include <iostream>

BoxControllerWebDataServer::BoxControllerWebDataServer(Wt::WServer& server): server_(server)
{
}

bool BoxControllerWebDataServer::connect(Client* client, const DataServerEventCallback& handleEvent)
{
	std::unique_lock<std::recursive_mutex> lock(mutex_);

	if (clients_.count(client) == 0) {
		ClientInfo clientInfo;

		clientInfo.sessionId = Wt::WApplication::instance()->sessionId();
		clientInfo.eventCallback = handleEvent;

		clients_[client] = clientInfo;

		return true;
	}
	else {
		return false;
	}
}

bool BoxControllerWebDataServer::disconnect(Client* client)
{
	std::unique_lock<std::recursive_mutex> lock(mutex_);
	return clients_.erase(client) == 1;
}

BoxControllerWebDataServer::UserSet BoxControllerWebDataServer::users()
{
	std::unique_lock<std::recursive_mutex> lock(mutex_);
	BoxControllerWebDataServer::UserSet result = users_;
	return result;
}

void BoxControllerWebDataServer::postDataServerEvent(const DataServerEvent& event)
{
	std::unique_lock<std::recursive_mutex> lock(mutex_);

	Wt::WApplication* app = Wt::WApplication::instance();

	for (ClientMap::const_iterator i = clients_.begin(); i != clients_.end();
		++i) {
		/*
		 * If the user corresponds to the current application, we directly
		 * call the call back method. This avoids an unnecessary delay for
		 * the update to the user causing the event.
		 *
		 * For other uses, we post it to their session. By posting the
		 * event, we avoid dead-lock scenarios, race conditions, and
		 * delivering the event to a session that is just about to be
		 * terminated.
		 */
		if (app && app->sessionId() == i->second.sessionId)
			i->second.eventCallback(event);
		else
			server_.post(i->second.sessionId,
				std::bind(i->second.eventCallback, event));
	}
}