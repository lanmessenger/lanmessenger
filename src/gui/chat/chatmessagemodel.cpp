// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 LAN Messenger.
#include "chatmessagemodel.h"

#include "core/shared.h"
#include "gui/chat/chatdefinitions.h"
#include "gui/chat/chattextguard.h"

namespace {

QString bodyText(XmlMessage& msg) {
  QString text = msg.data(XN_MESSAGE);
  if(text.isEmpty())
    text = msg.data(XN_GROUPMESSAGE);
  if(text.isEmpty())
    text = msg.data(XN_BROADCAST);
  return text;
}

QDateTime msgTime(XmlMessage& msg) {
  QDateTime t;
  t.setMSecsSinceEpoch(msg.header(XN_TIME).toLongLong());
  return t;
}

} // namespace

ChatMessageModel::ChatMessageModel(QObject* parent)
    : QAbstractListModel(parent) {
  messageLog.clear();
  sendFileMap.clear();
  receiveFileMap.clear();
  lastId = QString();
  hasData = false;
  outStyle = false;
}

int ChatMessageModel::rowCount(const QModelIndex& parent) const {
  if(parent.isValid())
    return 0;
  return messageLog.count() + (hasTyping ? 1 : 0);
}

QVariant ChatMessageModel::data(const QModelIndex& index, int role) const {
  if(!index.isValid() || index.row() < 0 || index.row() >= rowCount())
    return QVariant();
  const SingleMessage& msg = isTypingRow(index.row()) ? typingMsg : messageLog.at(index.row());
  XmlMessage xml = msg.message;
  switch(role) {
  case Qt::DisplayRole:
  case TextRole:
    return messageText(msg);
  case TypeRole:
    return int(msg.type);
  case UserIdRole:
    return msg.userId;
  case UserNameRole:
    return msg.userName;
  case TimeRole:
    return msgTime(xml);
  case FontRole: {
    //  Absent/unparsable XN_FONT: invalid marks the theme fallback.
    //  Parsed sizes clamp to [kMinFontPt, kMaxFontPt]; <= 0 maps to the
    //  theme size in rowFont.
    QString fontStr = xml.data(XN_FONT);
    if(fontStr.isEmpty())
      return QVariant();
    QFont f;
    if(!f.fromString(fontStr))
      return QVariant();
    return ChatTextGuard::sanitizeIncomingFont(f);
  }
  case ColorRole: {
    QColor c;
    c.setNamedColor(xml.data(XN_COLOR));
    return ChatTextGuard::sanitizeIncomingColor(c);
  }
  case AvatarRole:
    return participantAvatars.value(msg.userId, QString());
  case GroupedRole: {
    if(index.row() == 0)
      return false;
    //  Grouping by sender only (messagelog.cpp:546,581), no time window.
    return msg.userId == messageLog.at(index.row() - 1).userId &&
           (msg.type == MT_Message || msg.type == MT_PublicMessage ||
            msg.type == MT_GroupMessage);
  }
  case FileRole:
    return msg.type == MT_File || msg.type == MT_Folder;
  case OwnRole:
    return msg.userId == localId;
  case PreviewRole:
    //  No link previews at all (stage 1): always false.
    return false;
  case IdRole:
    return msg.id;
  case FileOpRole:
    return xml.data(XN_FILEOP);
  case FileModeRole:
    return xml.data(XN_MODE);
  case FileNameRole:
    return xml.data(XN_FILENAME);
  case FileSizeRole:
    return xml.data(XN_FILESIZE).toLongLong();
  default:
    break;
  }
  return QVariant();
}

QHash<int, QByteArray> ChatMessageModel::roleNames() const {
  return {{Qt::DisplayRole, "display"}, {TypeRole, "type"}, {UserIdRole, "userId"}, {UserNameRole, "userName"}, {TimeRole, "time"}, {TextRole, "text"}, {FontRole, "font"}, {ColorRole, "color"}, {AvatarRole, "avatar"}, {GroupedRole, "isGrouped"}, {FileRole, "isFile"}, {OwnRole, "isOwn"}, {PreviewRole, "preview"}, {IdRole, "id"}, {FileOpRole, "fileOp"}, {FileModeRole, "mode"}, {FileNameRole, "fileName"}, {FileSizeRole, "fileSize"}};
}

int ChatMessageModel::appendMessageLog(MessageType type, QString* lpszUserId, QString* lpszUserName,
                                       XmlMessage* pMessage, bool bReload) {
  if(!pMessage && type != MT_Error)
    return -1;

  //  Any append drops a stale typing row (messagelog parity).
  if(type != MT_ChatState)
    clearTypingRow();

  QString userId = lpszUserId ? *lpszUserId : QString();
  QString userName = lpszUserName ? *lpszUserName : QString();
  QString id;
  bool addToLog = true;
  MessageType storeType = type;

  switch(type) {
  case MT_Message:
  case MT_PublicMessage:
  case MT_GroupMessage:
    if(type == MT_PublicMessage || type == MT_GroupMessage) {
      if(userId != lastId)
        outStyle = !outStyle;
    }
    lastId = userId;
    break;
  case MT_Broadcast:
    lastId = QString();
    break;
  case MT_ChatState: {
    //  Ephemeral typing row: shown, never stored.
    if(!pMessage)
      return -1;
    ChatState state =
      (ChatState)Helper::indexOf(ChatStateNames, CS_Max, pMessage->data(XN_CHATSTATE));
    if(state != CS_Composing && state != CS_Paused) {
      clearTypingRow();
      return -1;
    }
    if(hasTyping && typingMsg.userId == userId &&
       typingMsg.message.data(XN_CHATSTATE) == ChatStateNames[state])
      return messageLog.count();
    clearTypingRow();
    typingMsg = SingleMessage(type, userId, userName, pMessage->clone(), QString());
    beginInsertRows(QModelIndex(), messageLog.count(), messageLog.count());
    hasTyping = true;
    endInsertRows();
    return messageLog.count();
  }
  case MT_Error:
    //  As in messageLog: send errors are not stored.
    addToLog = false;
    break;
  case MT_Failed:
    lastId = QString();
    break;
  case MT_File:
  case MT_Folder: {
    id = fileTempId(pMessage);
    QString fileId = pMessage->data(XN_FILEID);
    FileMode mode = (FileMode)Helper::indexOf(FileModeNames, FM_Max, pMessage->data(XN_MODE));
    FileOp op = (FileOp)Helper::indexOf(FileOpNames, FO_Max, pMessage->data(XN_FILEOP));
    if(mode == FM_Send)
      sendFileMap.insert(fileId, *pMessage);
    else
      receiveFileMap.insert(fileId, *pMessage);
    if(pMessage->data(XN_MODE) == QLatin1String("receive") && op == FO_Request) {
      bool autoFile = false;
      //  autoFile is decided by the window (as messagelog autoFile): model only stores.
      Q_UNUSED(autoFile);
    }
    //  storeType keeps the incoming type: MT_Folder renders its own caption.
    lastId = QString();
    break;
  }
  case MT_Join:
  case MT_Leave:
    lastId = QString();
    break;
  default:
    return -1;
  }

  if(!addToLog)
    return -1;

  int row = messageLog.count();
  beginInsertRows(QModelIndex(), row, row);
  XmlMessage clone = pMessage ? pMessage->clone() : XmlMessage();
  messageLog.append(SingleMessage(storeType, userId, userName, clone, id));
  hasData = true;
  endInsertRows();
  if(!bReload) {
    //  bReload is used on reload (as messagelog bReload=true): no extra logic.
  }
  return row;
}

void ChatMessageModel::updateFileMessage(FileMode mode, FileOp op, QString fileId) {
  QString tempId = fileTempId(mode, fileId);
  for(int i = 0; i < messageLog.count(); i++) {
    if(tempId.compare(messageLog.at(i).id) != 0)
      continue;
    SingleMessage msg = messageLog.at(i);
    XmlMessage xml = msg.message;
    xml.removeData(XN_FILEOP);
    xml.addData(XN_FILEOP, FileOpNames[op]);
    msg.message = xml;
    messageLog[i] = msg;
    if(mode == FM_Send && sendFileMap.contains(fileId)) {
      XmlMessage stored = sendFileMap.value(fileId);
      stored.removeData(XN_FILEOP);
      stored.addData(XN_FILEOP, FileOpNames[op]);
      sendFileMap.insert(fileId, stored);
    }
    if(mode == FM_Receive && receiveFileMap.contains(fileId)) {
      XmlMessage stored = receiveFileMap.value(fileId);
      stored.removeData(XN_FILEOP);
      stored.addData(XN_FILEOP, FileOpNames[op]);
      receiveFileMap.insert(fileId, stored);
    }
    QModelIndex idx = index(i, 0);
    emit dataChanged(idx, idx, {TextRole, Qt::DisplayRole, FileOpRole});
    break;
  }
}

void ChatMessageModel::updateUserName(QString* lpszUserId, QString* lpszUserName) {
  if(!lpszUserId || !lpszUserName)
    return;
  for(int i = 0; i < messageLog.count(); i++) {
    if(lpszUserId->compare(messageLog.at(i).userId) != 0)
      continue;
    SingleMessage msg = messageLog.at(i);
    msg.userName = *lpszUserName;
    messageLog[i] = msg;
    QModelIndex idx = index(i, 0);
    emit dataChanged(idx, idx, {UserNameRole, TextRole, Qt::DisplayRole});
  }
  //  No clear() and full rebuild (unlike reloadMessageLog).
}

void ChatMessageModel::updateAvatar(QString* lpszUserId, QString* lpszFilePath) {
  if(!lpszUserId || !lpszFilePath)
    return;
  participantAvatars.insert(*lpszUserId, *lpszFilePath);
  for(int i = 0; i < messageLog.count(); i++) {
    if(messageLog.at(i).userId != *lpszUserId)
      continue;
    QModelIndex idx = index(i, 0);
    emit dataChanged(idx, idx, {AvatarRole});
  }
}

void ChatMessageModel::abortPendingFileOperations() {
  for(auto it = sendFileMap.begin(); it != sendFileMap.end(); ++it) {
    FileOp op = (FileOp)Helper::indexOf(FileOpNames, FO_Max, it.value().data(XN_FILEOP));
    if(op == FO_Request)
      updateFileMessage(FM_Send, FO_Abort, it.value().data(XN_FILEID));
  }
  for(auto it = receiveFileMap.begin(); it != receiveFileMap.end(); ++it) {
    FileOp op = (FileOp)Helper::indexOf(FileOpNames, FO_Max, it.value().data(XN_FILEOP));
    if(op == FO_Request)
      updateFileMessage(FM_Receive, FO_Abort, it.value().data(XN_FILEID));
  }
}

void ChatMessageModel::clearLog() {
  if(messageLog.isEmpty() && !hasTyping)
    return;
  beginResetModel();
  messageLog.clear();
  sendFileMap.clear();
  receiveFileMap.clear();
  lastId = QString();
  outStyle = false;
  hasData = false;
  hasTyping = false;
  typingMsg = SingleMessage();
  endResetModel();
}

bool ChatMessageModel::requestFileAction(const QString& fileId, const QString& action,
                                         const QString& fileType, FileMode mode,
                                         XmlMessage* outMessage) {
  XmlMessage fileData;
  if(fileType.compare(QStringLiteral("file")) != 0 &&
     fileType.compare(QStringLiteral("folder")) != 0)
    return false;

  if(action == QLatin1String("accept")) {
    if(!receiveFileMap.contains(fileId))
      return false;
    fileData = receiveFileMap.value(fileId);
    outMessage->addData(XN_MODE, FileModeNames[FM_Receive]);
    outMessage->addData(XN_FILETYPE, FileTypeNames[FT_Normal]);
    outMessage->addData(XN_FILEOP, FileOpNames[FO_Accept]);
    outMessage->addData(XN_FILEID, fileData.data(XN_FILEID));
    outMessage->addData(XN_FILEPATH, fileData.data(XN_FILEPATH));
    outMessage->addData(XN_FILENAME, fileData.data(XN_FILENAME));
    outMessage->addData(XN_FILESIZE, fileData.data(XN_FILESIZE));
  } else if(action == QLatin1String("decline")) {
    if(!receiveFileMap.contains(fileId))
      return false;
    fileData = receiveFileMap.value(fileId);
    outMessage->addData(XN_MODE, FileModeNames[FM_Receive]);
    outMessage->addData(XN_FILETYPE, FileTypeNames[FT_Normal]);
    outMessage->addData(XN_FILEOP, FileOpNames[FO_Decline]);
    outMessage->addData(XN_FILEID, fileData.data(XN_FILEID));
  } else if(action == QLatin1String("cancel")) {
    fileData = (mode == FM_Receive) ? receiveFileMap.value(fileId) : sendFileMap.value(fileId);
    if(fileData.data(XN_FILEID).isEmpty())
      return false;
    outMessage->addData(XN_MODE, FileModeNames[mode]);
    outMessage->addData(XN_FILETYPE, FileTypeNames[FT_Normal]);
    outMessage->addData(XN_FILEOP, FileOpNames[FO_Cancel]);
    outMessage->addData(XN_FILEID, fileData.data(XN_FILEID));
  } else {
    return false;
  }

  FileOp op = (action == QLatin1String("accept"))
                ? FO_Accept
                : (action == QLatin1String("decline") ? FO_Decline : FO_Cancel);
  FileMode opMode = (action == QLatin1String("cancel")) ? mode : FM_Receive;
  updateFileMessage(opMode, op, fileId);
  return true;
}

const SingleMessage& ChatMessageModel::messageAt(int row) const {
  if(isTypingRow(row))
    return typingMsg;
  return messageLog.at(row);
}

bool ChatMessageModel::isTypingRow(int row) const {
  return hasTyping && row == messageLog.count();
}

const SingleMessage& ChatMessageModel::typingMessage() const {
  return typingMsg;
}

void ChatMessageModel::clearTypingRow() {
  if(!hasTyping)
    return;
  beginRemoveRows(QModelIndex(), messageLog.count(), messageLog.count());
  hasTyping = false;
  typingMsg = SingleMessage();
  endRemoveRows();
}

QList<SingleMessage> ChatMessageModel::messages() const {
  return messageLog;
}

void ChatMessageModel::setMessages(const QList<SingleMessage>& messages) {
  beginResetModel();
  messageLog = messages;
  lastId = QString();
  outStyle = false;
  hasData = !messageLog.isEmpty();
  hasTyping = false;
  typingMsg = SingleMessage();
  endResetModel();
}
QString ChatMessageModel::messageText(const SingleMessage& msg) const {
  XmlMessage xml = msg.message;
  switch(msg.type) {
  case MT_Message:
  case MT_PublicMessage:
  case MT_GroupMessage:
    return bodyText(xml);
  case MT_Broadcast:
    return tr("Broadcast message from %1: %2").arg(msg.userName, bodyText(xml));
  case MT_File:
  case MT_Folder: {
    QString kind = (msg.type == MT_File) ? tr("file") : tr("folder");
    bool incoming = xml.data(XN_MODE) == QLatin1String("receive");
    QString head = incoming ? tr("%1 sends you a %2:").arg(msg.userName, kind)
                            : tr("Sending '%1' to %2.").arg(xml.data(XN_FILENAME), msg.userName);
    QString fileLine = xml.data(XN_FILENAME);
    QString size = xml.data(XN_FILESIZE);
    if(!size.isEmpty())
      fileLine += QStringLiteral(" (") + Helper::formatSize(size.toLongLong()) +
                  QStringLiteral(")");
    QString tail = statusText(msg);
    if(!tail.isEmpty())
      fileLine += QStringLiteral(" ") + tail;
    return head + QStringLiteral("\n") + fileLine;
  }
  case MT_Failed:
    return tr("This message was not delivered to %1:\n%2")
      .arg(msg.userName, bodyText(xml));
  case MT_Join:
    return tr("%1 has joined this conversation").arg(msg.userName);
  case MT_Leave:
    return tr("%1 has left this conversation").arg(msg.userName);
  case MT_ChatState: {
    ChatState state =
      (ChatState)Helper::indexOf(ChatStateNames, CS_Max, xml.data(XN_CHATSTATE));
    if(state == CS_Composing)
      return tr("%1 is typing...").arg(msg.userName);
    if(state == CS_Paused)
      return tr("%1 has entered text").arg(msg.userName);
    return QString();
  }
  default:
    break;
  }
  return QString();
}

QString ChatMessageModel::plainText() const {
  //  Canonical plainText for e2e: model->data() joined, \n per message,
  //  sent+echo duplicates kept, links as text, smileys as source text without qrc:.
  QStringList lines;
  for(const SingleMessage& msg : messageLog) {
    QString t = messageText(msg);
    if(!t.isEmpty())
      lines.append(t);
  }
  return lines.join(QStringLiteral("\n"));
}

QString ChatMessageModel::fileTempId(FileMode mode, const QString& fileId) const {
  QString tempId = (mode == FM_Send) ? QStringLiteral("send") : QStringLiteral("receive");
  tempId.append(fileId);
  return tempId;
}

QString ChatMessageModel::fileTempId(XmlMessage* pMessage) const {
  QString fileId = pMessage->data(XN_FILEID);
  FileMode mode = (FileMode)Helper::indexOf(FileModeNames, FM_Max, pMessage->data(XN_MODE));
  return fileTempId(mode, fileId);
}

QString ChatMessageModel::statusText(const SingleMessage& msg) const {
  XmlMessage xml = msg.message;
  FileMode mode = (FileMode)Helper::indexOf(FileModeNames, FM_Max, xml.data(XN_MODE));
  FileOp op = (FileOp)Helper::indexOf(FileOpNames, FO_Max, xml.data(XN_FILEOP));
  switch(op) {
  case FO_Accept:
    return tr("Accepted");
  case FO_Decline:
    return tr("Declined");
  case FO_Cancel:
    return tr("Canceled");
  case FO_Error:
  case FO_Abort:
    return tr("Interrupted");
  case FO_Complete:
    return tr("Completed");
  case FO_Request:
    //  Accept/Decline/Cancel links are delegate interactivity, absent from
    //  plainText (S: plain-text only; rich-text and Copy Link are follow-ups).
    if(mode == FM_Receive)
      return QString();
    return QString();
  default:
    break;
  }
  return QString();
}
