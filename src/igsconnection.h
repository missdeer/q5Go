/*

 * igsconnection.h

 */

#ifndef IGSCONNECTION_H
#define IGSCONNECTION_H

#include <QObject>
#include <QTcpSocket>
#include <QString>

#include "igsinterface.h"

class QTextCodec;

#define MAX_LINESIZE 512

class IGSConnection : public QObject, public IGSInterface
{
	Q_OBJECT

	QWidget *m_lv_p, *m_lv_g;
public:
	IGSConnection(QWidget *, QWidget *);
	virtual ~IGSConnection();

	// Implementation of IGSInterface virtual functions
	virtual bool isConnected();
	virtual bool openConnection(const QString &host, unsigned port, const QString &user, const QString &pass);

	virtual bool closeConnection();
	virtual void sendTextToHost(QString txt, bool ignoreCodec=false);

	virtual void setTextCodec(QString codec);


signals:
	// for statistics reason
	void signal_setBytesIn(int);
	void signal_setBytesOut(int);

protected:
	virtual bool checkPrompt(const QString &);
//	void convertBlockToLines();
	
	void sendTextToApp(QString txt);

private slots:
	void OnHostFound();
	void OnConnected();
	void OnReadyRead();
	void OnConnectionClosed();
	void OnDelayedCloseFinish();

private:
	QTcpSocket *qsocket;
	QTextCodec *textCodec;

	int len_saved_data;
	char *saved_data;
	//struct USERINFO {
	QString username;
	QString password;
	//}  userInfo;

	enum {
		LOGIN,	// parse will search for login prompt
		PASSWORD,	// parse will search for password prompt
		SESSION,	// logged in
		AUTH_FAILED	// wrong user/pass  
	} authState;
};

#endif

