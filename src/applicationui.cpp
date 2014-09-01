/*
 * Copyright (c) 2011-2014 BlackBerry Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


/*
 *
 *
 * DON'T FORGET TO MANUALLY ADD THE QT NETWORK LIBRARY
 *
 * IF YOU'VE IMPORTED THIS PROJECT, IT'S ALREADY DONE.
 *
 * IF YOU COPY/PASTE CODE FROM THIS PROJECT, LEFT CLICK ON YOUR OWN PROJECT, SELECT
 * CONFIGURE -> ADD LIBRARY... -> NEXT -> Qt Network
 *
 *
 */




#include "applicationui.hpp"

#include <bb/cascades/Application>
#include <bb/cascades/QmlDocument>
#include <bb/cascades/AbstractPane>
#include <bb/cascades/LocaleHandler>

// Those are the include that we'll need later
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QFile>
#include <bb/system/SystemToast>

using namespace bb::cascades;

// using namespace and you don't have to specify bb::system::SystemToast, only SystemToast
using namespace bb::system;

ApplicationUI::ApplicationUI() :
        QObject()
{
    // prepare the localization
    m_pTranslator = new QTranslator(this);
    m_pLocaleHandler = new LocaleHandler(this);

    // Initiator for the QNetworkAccessManager
    m_networkAccessManager = new QNetworkAccessManager(this);

    bool res = QObject::connect(m_pLocaleHandler, SIGNAL(systemLanguageChanged()), this, SLOT(onSystemLanguageChanged()));
    // This is only available in Debug builds
    Q_ASSERT(res);
    // Since the variable is not used in the app, this is added to avoid a
    // compiler warning
    Q_UNUSED(res);

    // initial load
    onSystemLanguageChanged();

    // Create scene document from main.qml asset, the parent is set
    // to ensure the document gets destroyed properly at shut down.
    QmlDocument *qml = QmlDocument::create("asset:///main.qml").parent(this);

    // Connects the C++ part with QML
    qml->setContextProperty("_app", this);

    // Create root object for the UI
    AbstractPane *root = qml->createRootObject<AbstractPane>();

    // Set created root object as the application scene
    Application::instance()->setScene(root);
}

void ApplicationUI::onSystemLanguageChanged()
{
    QCoreApplication::instance()->removeTranslator(m_pTranslator);
    // Initiate, load and install the application translation files.
    QString locale_string = QLocale().name();
    QString file_name = QString("Download_%1").arg(locale_string);
    if (m_pTranslator->load(file_name, "app/native/qm")) {
        QCoreApplication::instance()->installTranslator(m_pTranslator);
    }
}

void ApplicationUI::download(const QString urlFromQml)
{
    // This function is called from QML

    // Create a request
    QUrl url = QUrl(urlFromQml);
    QNetworkRequest request(url);

    // Send the request and save the reply in replyGet pointer
    QNetworkReply* replyGet = m_networkAccessManager->get(request);

    // Connect the function onReadReply() to the finished() signal of the reply
    connect(replyGet, SIGNAL(finished()), this, SLOT(onReadReply()));
}

void ApplicationUI::onReadReply()
{
    // We got a reply!!!

    qDebug() << "We got a reply!!!";

    // Retrieve the reply
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

    // If reply exists
    if (reply) {
        // If the reply ain't an error reply
        if (reply->error() == QNetworkReply::NoError) {
            // If the reply length is more than 0 (if it's not empty)
            const int available = reply->bytesAvailable();
            if (available > 0) {
                // Save the reply to a variable
                const QByteArray buffer(reply->readAll());

                // Print the variable to the console, this is for debugging only
                qDebug() << "Buffer :" << buffer;

                // Retrieve the file name
                QString fileName = reply->url().toString().split("/").last().remove("/");

                // Create the new file and writes it
                QFile thisFile("/accounts/1000/shared/documents/" + (fileName.isEmpty() ? "downloadApp.file" : fileName));

                // Print the file path to the console, this is for debugging only
                qDebug() << "File name :" << thisFile.fileName();

                // Try to open the file
                if (thisFile.open(QIODevice::ReadWrite))
                {
                    qDebug() << "File was opened, writing to file";
                    // Write to the file
                    thisFile.write(buffer);
                    thisFile.flush();
                    thisFile.close();

                    // Warn the user that the file is now in the Documents folder
                    SystemToast* pToast = new SystemToast();
                    pToast->setBody(thisFile.fileName().split("/").last() + " saved to your Documents folder");
                    pToast->setPosition(SystemUiPosition::MiddleCenter);
                    pToast->show();
                }
                // Memory management
                thisFile.deleteLater();
            }
        } // end of : if (reply->error() == QNetworkReply::NoError)
        // Memory management
        reply->deleteLater();
    }  // end of : if (reply)
}
