#ifndef __APPLICATION__
#define __APPLICATION__

#include <boost/asio.hpp>

#include "LedgerMaster.h"
#include "UniqueNodeList.h"
#include "ConnectionPool.h"
#include "ScopedLock.h"
#include "LedgerAcquire.h"
#include "TransactionMaster.h"
#include "Wallet.h"
#include "Peer.h"
#include "NetworkOPs.h"
#include "WSDoor.h"
#include "TaggedCache.h"
#include "ValidationCollection.h"
#include "Suppression.h"
#include "SNTPClient.h"
#include "../database/database.h"
#include "JobQueue.h"
#include "RPCHandler.h"

class RPCDoor;
class PeerDoor;
typedef TaggedCache< uint256, std::vector<unsigned char> > NodeCache;

class DatabaseCon
{
protected:
	Database*				mDatabase;
	boost::recursive_mutex	mLock;

public:
	DatabaseCon(const std::string& name, const char *initString[], int countInit);
	~DatabaseCon();
	Database* getDB() { return mDatabase; }
	ScopedLock getDBLock() { return ScopedLock(mLock); }
};

class Application
{
	boost::asio::io_service			mIOService, mAuxService;
	boost::asio::io_service::work	mIOWork, mAuxWork;

	boost::recursive_mutex	mMasterLock;

	Wallet					mWallet;
	UniqueNodeList			mUNL;
	LedgerMaster			mMasterLedger;
	LedgerAcquireMaster		mMasterLedgerAcquire;
	TransactionMaster		mMasterTransaction;
	NetworkOPs				mNetOps;
	NodeCache				mTempNodeCache;
	ValidationCollection	mValidations;
	SuppressionTable		mSuppressions;
	HashedObjectStore		mHashedObjectStore;
	SNTPClient				mSNTPClient;
	JobQueue				mJobQueue;
	RPCHandler				mRPCHandler;

	DatabaseCon				*mRpcDB, *mTxnDB, *mLedgerDB, *mWalletDB, *mHashNodeDB, *mNetNodeDB;

	ConnectionPool			mConnectionPool;
	PeerDoor*				mPeerDoor;
	RPCDoor*				mRPCDoor;
	WSDoor*					mWSPublicDoor;
	WSDoor*					mWSPrivateDoor;

	uint256					mNonce256;
	std::size_t				mNonceST;

	boost::asio::deadline_timer	mSweepTimer;

	std::map<std::string, Peer::pointer> mPeerMap;
	boost::recursive_mutex	mPeerMapLock;

	void startNewLedger();
	void loadOldLedger();

public:
	Application();
	~Application();

	ConnectionPool& getConnectionPool()				{ return mConnectionPool; }

	UniqueNodeList& getUNL()						{ return mUNL; }

	Wallet& getWallet()								{ return mWallet ; }
	NetworkOPs& getOPs()							{ return mNetOps; }

	boost::asio::io_service& getIOService()			{ return mIOService; }
	boost::asio::io_service& getAuxService()		{ return mAuxService; }

	LedgerMaster& getMasterLedger()					{ return mMasterLedger; }
	LedgerAcquireMaster& getMasterLedgerAcquire()	{ return mMasterLedgerAcquire; }
	TransactionMaster& getMasterTransaction()		{ return mMasterTransaction; }
	NodeCache& getTempNodeCache()					{ return mTempNodeCache; }
	HashedObjectStore& getHashedObjectStore()		{ return mHashedObjectStore; }
	ValidationCollection& getValidations()			{ return mValidations; }
	JobQueue& getJobQueue()							{ return mJobQueue; }
	SuppressionTable& getSuppression()				{ return mSuppressions; }
	RPCHandler& getRPCHandler()						{ return mRPCHandler; }
	boost::recursive_mutex& getMasterLock()			{ return mMasterLock; }


	bool isNew(const uint256& s)					{ return mSuppressions.addSuppression(s); }
	bool isNew(const uint256& s, uint64 p)			{ return mSuppressions.addSuppressionPeer(s, p); }
	bool isNew(const uint256& s, uint64 p, int& f)	{ return mSuppressions.addSuppressionPeer(s, p, f); }
	bool isNewFlag(const uint256& s, int f)			{ return mSuppressions.setFlag(s, f); }
	bool running()									{ return mTxnDB != NULL; }
	bool getSystemTimeOffset(int& offset)			{ return mSNTPClient.getOffset(offset); }

	DatabaseCon* getRpcDB()			{ return mRpcDB; }
	DatabaseCon* getTxnDB()			{ return mTxnDB; }
	DatabaseCon* getLedgerDB()		{ return mLedgerDB; }
	DatabaseCon* getWalletDB()		{ return mWalletDB; }
	DatabaseCon* getHashNodeDB()	{ return mHashNodeDB; }
	DatabaseCon* getNetNodeDB()		{ return mNetNodeDB; }

	uint256 getNonce256()			{ return mNonce256; }
	std::size_t getNonceST()		{ return mNonceST; }

	void run();
	void stop();
	void sweep();
};

extern Application* theApp;

#endif
// vim:ts=4