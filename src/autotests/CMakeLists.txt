set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
include_directories(
    BEFORE
    ${kmail_SOURCE_DIR}
    ${kmail_BINARY_DIR}
    )

########### dbus test ###############
set( kmail_displaymenu_source displaymessageformatactionmenutest.cpp ../widgets/displaymessageformatactionmenu.cpp )
add_executable( displaymessageformatactionmenutest ${kmail_displaymenu_source} )
add_test(NAME displaymessageformatactionmenutest COMMAND displaymessageformatactionmenutest )
ecm_mark_as_test(kmail-displaymessageformatactionmenutest)
target_link_libraries( displaymessageformatactionmenutest Qt5::Test  KF5::MessageViewer KF5::I18n)

set( kmail_identityaddvcarddialog_source identityaddvcarddialogtest.cpp ../identity/identityaddvcarddialog.cpp )
add_executable( identityaddvcarddialogtest  ${kmail_identityaddvcarddialog_source})
ecm_mark_as_test(kmail-identityaddvcarddialogtest)
target_link_libraries( identityaddvcarddialogtest Qt5::Test Qt5::Widgets KF5::Completion KF5::KIOWidgets KF5::I18n)

set( kmail_statusbarlabeltoggledstate_source statusbarlabeltoggledstatetest.cpp ../widgets/statusbarlabeltoggledstate.cpp ../kmail_debug.cpp)
add_executable(statusbarlabeltoggledstatetest ${kmail_statusbarlabeltoggledstate_source})
ecm_mark_as_test(kmail-statusbarlabeltoggledstatetest)
target_link_libraries( statusbarlabeltoggledstatetest Qt5::Test KF5::KIOWidgets KF5::I18n)


set( kmail_createfollowupreminderonexistingmessagejobtest_source createfollowupreminderonexistingmessagejobtest.cpp ../job/createfollowupreminderonexistingmessagejob.cpp  ../kmail_debug.cpp)
add_executable( createfollowupreminderonexistingmessagejobtest ${kmail_createfollowupreminderonexistingmessagejobtest_source})
ecm_mark_as_test(kmail_createfollowupreminderonexistingmessagejobtest)
target_link_libraries( createfollowupreminderonexistingmessagejobtest Qt5::Test Qt5::Widgets KF5::AkonadiCore KF5::CalendarCore KF5::Mime KF5::FollowupReminder KF5::MessageComposer)


set( kmail_cryptostateindicatorwidgettest_source cryptostateindicatorwidgettest.cpp ../editor/widgets/cryptostateindicatorwidget.cpp)
add_executable( cryptostateindicatorwidgettest ${kmail_cryptostateindicatorwidgettest_source})
add_test(NAME cryptostateindicatorwidgettest COMMAND cryptostateindicatorwidgettest)
ecm_mark_as_test(cryptostateindicatorwidgettest)
target_link_libraries( cryptostateindicatorwidgettest Qt5::Test  KF5::MessageCore Qt5::Widgets KF5::ConfigWidgets KF5::I18n)

set( kmail_kactionmenutransporttest_source kactionmenutransporttest.cpp ../widgets/kactionmenutransport.cpp)
add_executable( kactionmenutransporttest ${kmail_kactionmenutransporttest_source})
add_test(NAME kactionmenutransporttest COMMAND kactionmenutransporttest)
ecm_mark_as_test(kactionmenutransporttest)
target_link_libraries( kactionmenutransporttest Qt5::Test  KF5::MailTransportAkonadi KF5::WidgetsAddons KF5::I18n KF5::ConfigGui)

set(KDEPIMLIBS_RUN_ISOLATED_TESTS TRUE)
set(KDEPIMLIBS_RUN_SQLITE_ISOLATED_TESTS TRUE)

add_akonadi_isolated_test_advanced( tagselectdialogtest.cpp  "../tag/tagselectdialog.cpp;../kmail_debug.cpp" "${KDEPIMLIBS_AKONADI_LIBS};kmailprivate;KF5::MailCommon;KF5::Libkdepim;KF5::ItemViews;KF5::TemplateParser;KF5::XmlGui;KF5::Completion;KF5::I18n")

add_akonadi_isolated_test_advanced(kmcommandstest.cpp "../kmcommands.cpp;../util.cpp;../secondarywindow.cpp;../undostack.cpp;../kmail_debug.cpp;../job/handleclickedurljob.cpp;../job/createreplymessagejob.cpp;../job/createforwardmessagejob.cpp"
	"Qt5::Test;Qt5::Widgets;KF5::AkonadiCore;KF5::Bookmarks;KF5::ConfigWidgets;KF5::Contacts;KF5::I18n;KF5::IconThemes;KF5::IdentityManagement;KF5::KIOCore;KF5::KIOFileWidgets;KF5::MessageCore;KF5::MessageComposer;KF5::MessageList;KF5::MessageViewer;KF5::MailCommon;KF5::MailTransportAkonadi;KF5::Libkdepim;KF5::TemplateParser;kmailprivate")
