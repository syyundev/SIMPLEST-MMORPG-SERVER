#pragma once

class Session;


void Process_CS_LOGIN_PACKET(const std::shared_ptr<Session>& session, const CS_LOGIN_PACKET& recvPkt);
void Process_CS_MOVE_PACKET(const std::shared_ptr<Session>& session, const CS_MOVE_PACKET& recvPkt);
