/******************************************************************************
 *   Copyright (C) 2011-2015 Frank Osterfeld <frank.osterfeld@gmail.com>      *
 *                                                                            *
 * This program is distributed in the hope that it will be useful, but        *
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE. For licensing and distribution        *
 * details, check the accompanying file 'COPYING'.                            *
 *****************************************************************************/
#ifndef KEYCHAIN_H
#define KEYCHAIN_H

#include <QtCore/QObject>
#include <QtCore/QString>

class QSettings;

#define QTKEYCHAIN_VERSION 0x000100

/**
 * Error codes
 */
enum Error {
	NoError=0, /**< No error occurred, operation was successful */
	EntryNotFound, /**< For the given key no data was found */
	CouldNotDeleteEntry, /**< Could not delete existing secret data */
	AccessDeniedByUser, /**< User denied access to keychain */
	AccessDenied, /**< Access denied for other reasons */
	NoBackendAvailable, /**< No platform-specific keychain service available */
	NotImplemented, /**< Not implemented on platform */
	OtherError /**< Something else went wrong (errorString() might provide details) */
};

class JobExecutor;
class JobPrivate;

/**
 * @brief Abstract base class for all QKeychain jobs.
 */
class vmKeyChain : public QObject {
	Q_OBJECT
public:
        ~vmKeyChain();

	/**
	 * @return The QSettings instance used as plaintext storage if insecureFallback() is true.
	 * @see setSettings()
	 * @see insecureFallback()
	 */
	QSettings* settings() const;

	/**
	 * @return Set the QSettings instance that will be used as plaintext storage if insecureFallback() is true.
	 * @see settings()
	 * @see insecureFallback()
	 */
	void setSettings( QSettings* settings );

	/**
	 * Call this method to start the job.
	 * Typically you want to connect some slot to the finished() signal first:
	 *
	 * \code
	 * SomeClass::startJob()
	 * {
         *     connect(job, &vmKeyChain::finished, this, &SomeClass::slotJobFinished);
	 *     job->start();
	 * }
	 *
         * SomeClass::slotJobFinished(vmKeyChain *job)
	 * {
	 *     if (job->error()) {
	 *         // handle error
	 *     } else {
	 *         // do job-specific stuff
	 *     }
	 * }
	 * \endcode
	 *
	 * @see finished()
	 */
	void start();

	QString service() const;

	/**
	 * @note Call this method only after finished() has been emitted.
	 * @return The error code of the job (0 if no error).
	 */
	Error error() const;

	/**
	 * @return An error message that might provide details if error() returns OtherError.
	 */
	QString errorString() const;

	/**
	 * @return Whether this job autodeletes itself once finished() has been emitted. Default is true.
	 * @see setAutoDelete()
	 */
	bool autoDelete() const;

	/**
	 * Set whether this job should autodelete itself once finished() has been emitted.
	 * @see autoDelete()
	 */
	void setAutoDelete( bool autoDelete );

	/**
	 * @return Whether this job will use plaintext storage on unsupported platforms. Default is false.
	 * @see setInsecureFallback()
	 */
	bool insecureFallback() const;

	/**
	 * Set whether this job should use plaintext storage on unsupported platforms.
	 * @see insecureFallback()
	 */
	void setInsecureFallback( bool insecureFallback );

	/**
	 * @return The string used as key by this job.
	 * @see setKey()
	 */
	QString key() const;

	/**
	 * Set the @p key that this job will use to read or write data from/to the keychain.
	 * The key can be an empty string.
	 * @see key()
	 */
	void setKey( const QString& key );

	void emitFinished();
	void emitFinishedWithError(Error, const QString& errorString);

Q_SIGNALS:
	/**
	 * Emitted when this job is finished.
	 * You can connect to this signal to be notified about the job's completion.
	 * @see start()
	 */
        void finished( vmKeyChain* );

protected:
        explicit vmKeyChain( JobPrivate *q, QObject* parent=0 );
	Q_INVOKABLE void doStart();

private:
	void setError( Error error );
	void setErrorString( const QString& errorString );

	void scheduledStart();

protected:
	JobPrivate* const d;

friend class JobExecutor;
friend class JobPrivate;
friend class ReadPasswordJobPrivate;
friend class WritePasswordJobPrivate;
friend class DeletePasswordJobPrivate;
};

class ReadPasswordJobPrivate;

/**
 * @brief vmKeyChain for reading secrets from the keychain.
 * You can use a ReadPasswordJob to read passwords or binary data from the keychain.
 * This job requires a "service" string, which is basically a namespace of keys within the keychain.
 * This means that you can read all the pairs <key, secret> stored in the same service string.
 */
class ReadPasswordJob : public vmKeyChain {
	Q_OBJECT
public:
	/**
	 * Create a new ReadPasswordJob.
	 * @param service The service string used by this job (can be empty).
	 * @param parent The parent of this job.
	 */
	explicit ReadPasswordJob( const QString& service, QObject* parent=0 );
	~ReadPasswordJob();

	/**
	 * @return The binary data stored as value of this job's key().
         * @see vmKeyChain::key()
	 */
	QByteArray binaryData() const;

	/**
	 * @return The string stored as value of this job's key().
         * @see vmKeyChain::key()
	 * @warning Returns meaningless data if the data was stored as binary data.
	 * @see WritePasswordJob::setTextData()
	 */
	QString textData() const;

private:
    friend class ReadPasswordJobPrivate;
};

class WritePasswordJobPrivate;

/**
 * @brief vmKeyChain for writing secrets to the keychain.
 * You can use a WritePasswordJob to store passwords or binary data in the keychain.
 * This job requires a "service" string, which is basically a namespace of keys within the keychain.
 * This means that you can store different pairs <key, secret> under the same service string.
 */
class WritePasswordJob : public vmKeyChain {
	Q_OBJECT
public:
	/**
	 * Create a new WritePasswordJob.
	 * @param service The service string used by this job (can be empty).
	 * @param parent The parent of this job.
	 */
	explicit WritePasswordJob( const QString& service, QObject* parent=0 );
	~WritePasswordJob();

	/**
	 * Set the @p data that the job will store in the keychain as binary data.
	 * @warning setBinaryData() and setTextData() are mutually exclusive.
	 */
	void setBinaryData( const QByteArray& data );

	/**
	 * Set the @p data that the job will store in the keychain as string.
	 * Typically @p data is a password.
	 * @warning setBinaryData() and setTextData() are mutually exclusive.
	 */
	void setTextData( const QString& data );

private:

    friend class WritePasswordJobPrivate;
};

class DeletePasswordJobPrivate;

/**
 * @brief vmKeyChain for deleting secrets from the keychain.
 * You can use a DeletePasswordJob to delete passwords or binary data from the keychain.
 * This job requires a "service" string, which is basically a namespace of keys within the keychain.
 * This means that you can delete all the pairs <key, secret> stored in the same service string.
 */
class DeletePasswordJob : public vmKeyChain {
	Q_OBJECT
public:
	/**
	 * Create a new DeletePasswordJob.
	 * @param service The service string used by this job (can be empty).
	 * @param parent The parent of this job.
	 */
	explicit DeletePasswordJob( const QString& service, QObject* parent=0 );
	~DeletePasswordJob();

private:
    friend class DeletePasswordJobPrivate;
};


#endif
