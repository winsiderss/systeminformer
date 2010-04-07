; *** Inno Setup version 5.1.11+ Greek messages ***
;
; Note: When translating this text, do not add periods (.) to the end of
; messages that didn't have them already, because on those messages Inno
; Setup adds the periods automatically (appending a period would result in
; two periods being displayed).
;
; Translated by Anastasis Chatzioglou
;               http://anasto.go.to
;               baldycom@hotmail.com
;

[LangOptions]
LanguageName=Greek
LanguageID=$408
LanguageCodePage=1253
; If the language you are translating to requires special font faces or
; sizes, uncomment any of the following entries and change them accordingly.
;DialogFontName=MS Shell Dlg
;DialogFontSize=8
;DialogFontStandardHeight=13
;TitleFontName=Arial
;TitleFontSize=29
;WelcomeFontName=Verdana
;WelcomeFontSize=12
;CopyrightFontName=Arial
;CopyrightFontSize=8
DialogFontName=MS Shell Dlg
DialogFontSize=8
;4.1.4+
;DialogFontStandardHeight=13
TitleFontName=Arial
TitleFontSize=29
WelcomeFontName=Arial
WelcomeFontSize=12
CopyrightFontName=Arial
CopyrightFontSize=8

[Messages]
; *** Application titles
SetupAppTitle=Εγκατάσταση
SetupWindowTitle=Εγκατάσταση - %1
UninstallAppTitle=Απεγκατάσταση
UninstallAppFullTitle=%1 Απεγκατάσταση
; 2.0.x
;DefaultUninstallIconName=Απεγκατάσταση %1

; *** Misc. common
InformationTitle=Πληροφορίες
ConfirmTitle=Επιβεβαίωση
ErrorTitle=Σφάλμα

; *** SetupLdr messages
SetupLdrStartupMessage=Θα εκτελεστεί η εγκατάσταση του %1. Θέλετε να συνεχίσετε;
LdrCannotCreateTemp=Σφάλμα στη δημιουργία προσωρινού αρχείου. Η εγκατάσταση θα τερματιστεί τώρα.
LdrCannotExecTemp=Σφάλμα στην εκτέλεση αρχείου στον προσωρινό κατάλογο. Η εγκατάσταση τερματίζεται.

; *** Startup error messages
LastErrorMessage=%1.%n%nΣφάλμα %2: %3
SetupFileMissing=Δεν βρίσκεται το αρχείο %1 στον κατάλογο εγκατάστασης. Ίσως χρειάζεται να προμηθευτείτε ένα νέο αντίγραφο του προγράμματος.
SetupFileCorrupt=Το αρχείο εγκατάστασης είναι κατεστραμμένο. Ίσως χρειάζεται να προμηθευτείτε ένα νέο αντίγραφο του προγράμματος.
SetupFileCorruptOrWrongVer=Το αρχείο εγκατάστασης είναι κατεστραμμένο η είναι σε λάθος έκδοση. Ίσως χρειάζεται να προμηθευτείτε ένα νέο αντίγραφο του προγράμματος.
NotOnThisPlatform=Αυτό το πρόγραμμα δεν μπορεί να εκτελεστεί σε %1.
OnlyOnThisPlatform=Αυτό το πρόγραμμα εκτελείται μόνο σε %1.
; 5.1.0+
OnlyOnTheseArchitectures=Αυτό το πρόγραμμα μπορεί να εγκατασταθεί μονό σε Windows σχεδιασμένα για επεξεργαστές με αρχιτεκτονική:%n%n%1
MissingWOW64APIs=Η έκδοση των Windows που εκτελείται δεν διαθέτει λειτουργικότητα 64-bit. Για να διορθωθεί το πρόβλημα εγκατέστησε το Service Pack %1.
WinVersionTooLowError=Αυτό το πρόγραμμα απαιτεί %1 έκδοση η νεότερη.
WinVersionTooHighError=Αυτό το πρόγραμμα δεν μπορεί να εκτελεστεί σε %1 έκδοση η νεότερη.
AdminPrivilegesRequired=Πρέπει να είστε ο Διαχειριστής συστήματος για να εγκαταστήσετε αυτό το πρόγραμμα.
PowerUserPrivilegesRequired=Πρέπει να είστε ο Διαχειριστής συστήματος  ή Power User για να εγκαταστήσετε αυτό το πρόγραμμα.
SetupAppRunningError=Η εγκατάσταση εντόπισε ότι εκτελείται η εφαρμογή %1. Παρακαλώ κλείστε την εφαρμογή τώρα και πατήστε Εντάξει για να συνεχίσετε.
UninstallAppRunningError=Η απεγκατάσταση εντόπισε ότι εκτελείται η εφαρμογή %1. Παρακαλώ κλείστε την εφαρμογή τώρα και πατήστε Εντάξει για να συνεχίσετε.

; *** Misc. errors
ErrorCreatingDir=Η εγκατάσταση δεν μπορεί να δημιουργήσει τον κατάλογο %1.
ErrorTooManyFilesInDir=Δεν μπορεί να δημιουργηθεί ένα αρχείο στον κατάλογο "%1" επειδή ήδη περιέχει πολλά αρχεία.

; *** Setup common messages
ExitSetupTitle=Τέλος Εγκατάστασης.
ExitSetupMessage=Η εγκατάσταση δεν έχει τελειώσει. Αν τη σταματήσετε τώρα το πρόγραμμα που προσπαθήσατε να εγκαταστήσετε δεν θα λειτουργεί.%n%nΜπορείτε να εκτελέσετε ξανά την εγκατάσταση αργότερα.
AboutSetupMenuItem=&Σχετικά με την Εγκατάσταση...
AboutSetupTitle=Σχετικά με την Εγκατάσταση.
AboutSetupMessage=%1 έκδοση %2%n%3%n%n%1 προσωπική σελίδα%n%4
AboutSetupNote=Anasto
; 5.1.0+
TranslatorNote=Anastasis Chatzioglou - baldycom@hotmail.com

; *** Buttons
ButtonBack=< &Πίσω
ButtonNext=&Επόμενο >
ButtonInstall=&Εγκατάσταση
ButtonOK=Ε&ντάξει
ButtonCancel=&Ακυρο
ButtonYes=Ν&αι
ButtonYesToAll=Ναι σε &Ολα
ButtonNo=Ο&χι
ButtonNoToAll=Οχι &σε ολα
ButtonFinish=&Τέλος
ButtonBrowse=&Αναζήτηση...
;4.1.3
ButtonWizardBrowse=&Εύρεση...
ButtonNewFolder=&Δημιουργία νέου φακέλου

; *** "Select Language" dialog messages
; 4.0.x
SelectLanguageTitle=Επιλογή της γλώσσας εγκατάστασης
SelectLanguageLabel=Επιλογή της γλώσσας για χρήση κατά την διάρκεια της εγκατάστασης:


; *** Common wizard text
ClickNext=Πατήστε Επόμενο για να συνεχίσετε ή ’κυρο για να τερματίσετε την εγκατάσταση.
; 2.0.x
;ClickNextModern=Πατήστε Επόμενο για να συνεχίσετε ή ’κυρο για να τερματίσετε την εγκατάσταση.
;;; - anasto -
;4.1.3
BrowseDialogTitle=Εύρεση φακέλου
BrowseDialogLabel=Επιλέξτε φάκελο στην λίστα και μετά πατήστε OK.
NewFolderName=Νέος φάκελος

; *** "Welcome" wizard page
WelcomeLabel1=Καλωσορίσατε στην εγκατάσταση του [name].
WelcomeLabel2=Θα γίνει εγκατάσταση του [name/ver] στον υπολογιστή σας.%n%nΠριν συνεχίσετε σας συνιστούμε να κλείσετε κάθε άλλη εφαρμογή που πιθανόν εκτελείτε.

; *** "Password" wizard page
WizardPassword=Εισαγωγή Κωδικού
PasswordLabel1=Αυτή η εγκατάσταση χρειάζεται κωδικό για να εκτελεστεί.
PasswordLabel3=Παρακαλώ δώστε τον κωδικό σας και πατήστε Επόμενο για να συνεχίσετε.
PasswordEditLabel=&Κωδικός:
IncorrectPassword=Ο κωδικός που δώσατε είναι λάθος. Ξαναπροσπαθήστε.

; *** "License Agreement" wizard page
WizardLicense=Αδεια Χρήσης
LicenseLabel=Παρακαλώ διαβάστε προσεκτικά τις παρακάτω πληροφορίες πριν συνεχίσετε.
; 2.0.x
;LicenseLabel1=Παρακαλώ διαβάστε προσεκτικά τις παρακάτω πληροφορίες πριν συνεχίσετε. Χρησιμοποιήστε την μπάρα κύλισης για να δείτε όλο το κείμενο.
;LicenseLabel2=Αποδέχεστε τους όρους της ’δειας Χρήσης; Αν επιλέξετε όχι η εγκατάσταση θα τερματιστεί. Για να συνεχιστεί η εγκατάσταση πρέπει να αποδέχεστε τους όρους της ’δειας Χρήσης.
LicenseLabel3=Παρακαλώ διαβάστε προσεκτικά τις παρακάτω πληροφορίες πριν συνεχίσετε. Πρέπει να αποδέχεστε τους όρους της ’δειας Χρήσης πριν να συνεχίσετε την εγκατάσταση.
LicenseAccepted=&Δέχομαι τους όρους της ’δειας Χρήσης
LicenseNotAccepted=Δεν &αποδέχομαι τους όρους της ’δειας Χρήσης

; *** "Information" wizard pages
WizardInfoBefore=Πληροφορίες
InfoBeforeLabel=Παρακαλώ διαβάστε προσεκτικά τις παρακάτω πληροφορίες πριν συνεχίσετε.
InfoBeforeClickLabel=Αν είστε έτοιμοι να συνεχίσετε πατήστε Επόμενο.
WizardInfoAfter=Πληροφορίες
InfoAfterLabel=Παρακαλώ διαβάστε προσεκτικά τις παρακάτω πληροφορίες πριν συνεχίσετε.
InfoAfterClickLabel=Αν είστε έτοιμοι να συνεχίσετε πατήστε Επόμενο.

; *** "User Information" wizard page
WizardUserInfo=Πληροφορίες για τον Χρήστη
UserInfoDesc=Παρακαλώ δώστε τις πληροφορίες.
UserInfoName=&Ονομα Χρήστη:
UserInfoOrg=&Εταιρεία:
UserInfoSerial=&Σειριακό Αριθμό:
UserInfoNameRequired=Πρέπει να δώσετε όνομα.

; *** "Select Destination Location" wizard page
; 4.0.x
WizardSelectDir=Επιλέξτε τον κατάλογο που θα εγκατασταθεί το πρόγραμμα.
SelectDirDesc=Πού θα εγκατασταθεί το [name];
;SelectDirLabel=Επιλέξτε τον κατάλογο που θα εγκατασταθεί το πρόγραμμα. Πατήστε Επόμενο για να συνεχίσετε.
DiskSpaceMBLabel=Αυτό το πρόγραμμα χρειάζεται [mb] MB χώρο στον δίσκο.
ToUNCPathname=Η εγκατάσταση δεν μπορεί να γίνει σε δίσκο δικτύου. Αν θέλετε να γίνει η εγκατάσταση σε δίσκο δικτύου πρέπει να ορίσετε αυτόν το δίσκο.
InvalidPath=Δώστε την πλήρη διαδρομή.%nπαράδειγμα:%n%nC:\APP
InvalidDrive=Ο τοπικός δίσκος η ο δίσκος δικτύου που επιλέξατε δεν υπάρχει η δεν είναι προσβάσιμος. Επιλέξτε άλλον.
DiskSpaceWarningTitle=Δεν υπάρχει αρκετός χώρος στο δίσκο.
DiskSpaceWarning=Η εγκατάσταση χρειάζεται τουλάχιστον %1 KB ελεύθερο χώρο στο δίσκο αλλά ο επιλεγμένος οδηγός διαθέτει μόνον %2 KB.%n%nΘέλετε να συνεχίσετε οπωσδήποτε;
BadDirName32=Ονόματα καταλόγων δεν μπορούν να περιέχουν κάποιον από τους παρακάτω χαρακτήρες:%n%n%1
DirExistsTitle=Ο κατάλογος υπάρχει.
DirExists=Ο κατάλογος:%n%n%1%n%nυπάρχει ήδη. Θέλετε να γίνει η εγκατάσταση σε αυτόν τον κατάλογο;
DirDoesntExistTitle=Ο κατάλογος δεν υπάρχει.
DirDoesntExist=Ο κατάλογος:%n%n%1%n%nδεν υπάρχει. Θέλετε να δημιουργηθεί;
;4.1.3
InvalidDirName=Λάθος όνομα φακέλου.
;4.1.5
DirNameTooLong=Το όνομα του φακέλου είναι πολύ μεγάλο.
;4.1.8
;SelectDirLabel2=Το [name] θα εγκατασταθεί στον ακόλουθο φάκελο.%n%nΓια συνέχεια πατήστε Επόμενο. Αν θέλετε άλλο φάκελο, πατήστε Εύρεση.
SelectDirLabel3=Το [name] θα εγκατασταθεί στον ακόλουθο φάκελο.
SelectDirBrowseLabel=Για συνέχεια πατήστε Επόμενο. Αν θέλετε άλλο φάκελο, πατήστε Εύρεση.

; *** "Select Components" wizard page
WizardSelectComponents=Επιλογή Συστατικών
SelectComponentsDesc=Ποια συστατικά θέλετε να εγκατασταθούν;
SelectComponentsLabel2=Επιλέξτε τα συστατικά που θέλετε να εγκαταστήσετε και πατήστε Επόμενο για συνέχεια της εγκατάστασης.
FullInstallation=Πλήρης Εγκατάσταση.
; if possible don't translate 'Compact' as 'Minimal' (I mean 'Minimal' in your language)
CompactInstallation=Περιορισμένη Εγκατάσταση.
CustomInstallation=Προσαρμοσμένη Εγκατάσταση.
NoUninstallWarningTitle=Τα συστατικά υπάρχουν.
NoUninstallWarning=Η εγκατάσταση διαπίστωσε ότι τα παρακάτω συστατικά είναι ήδη εγκατεστημένα στον υπολογιστή σας:%n%n%1
ComponentSize1=%1 KB
ComponentSize2=%1 MB
ComponentsDiskSpaceMBLabel=Η συγκεκριμένη επιλογή απαιτεί τουλάχιστον [mb] MB ελεύθερο χώρο στον δίσκο.

; *** "Select Additional Tasks" wizard page
WizardSelectTasks=Επιλογή Περαιτέρω Ενεργειών
SelectTasksDesc=Ποιές επιπλέον ενέργειες θέλετε να γίνουν;
SelectTasksLabel2=Επιλέξτε τις επιπλέον ενέργειες που θέλετε να γίνουν κατά την εγκατάσταση του [name] και πατήστε Επόμενο.

; *** "Select Start Menu Folder" wizard page
; 2.0.x
;ReadyMemoTasks=Επιπλέον Ενέργειες:
WizardSelectProgramGroup=Επιλογή Καταλόγου Στο Μενού Εκκίνηση.
SelectStartMenuFolderDesc=Πού θα τοποθετηθούν οι συντομεύσεις του προγράμματος;
; 4.0.x
;SelectStartMenuFolderLabel=Επιλέξτε τον κατάλογο στο μενού εκκίνησης στον οποίο θέλετε δημιουργηθούν οι συντομεύσεις του προγράμματος και πατήστε Επόμενο.
; 5.1.0+
;NoIconsCheck=&Χωρίς δημιουργία εικονιδίων
MustEnterGroupName=Πρέπει να δώσετε το όνομα ενός καταλόγου.
BadGroupName=Ονόματα καταλόγων δεν μπορούν να περιέχουν κάποιον από τους παρακάτω χαρακτήρες:%n%n%1
NoProgramGroupCheck2=&Χωρίς δημιουργία καταλόγου στο μενού εκκίνηση.
;4.1.3
InvalidGroupName=Το όνομα του group δεν είναι σωστό.
;4.1.4+
GroupNameTooLong=Το όνομα του group ειναι πολύ μεγάλο.
;4.1.8
;SelectStartMenuFolderLabel2=Η εγκατάσταση θα δημιουργήσει τις συντομεύσεις του προγράμματος στην ακόλουθη ομάδα.%n%nΓια συνέχεια, πατήστε Επόμενο. Αν θέλετε άλλη ομάδα, πατήστε εύρεση.
SelectStartMenuFolderLabel3=Η εγκατάσταση θα δημιουργήσει τις συντομεύσεις του προγράμματος στην ακόλουθη ομάδα.
SelectStartMenuFolderBrowseLabel=Για συνέχεια, πατήστε Επόμενο. Αν θέλετε άλλη ομάδα, πατήστε εύρεση.


; *** "Ready to Install" wizard page
WizardReady=Έτοιμος για εγκατάσταση
ReadyLabel1=Η εγκατάσταση του [name] είναι έτοιμη να εκτελεστεί στον υπολογιστή σας.
ReadyLabel2a=Πατήστε Εγκατάσταση για να συνεχίσετε ή Πίσω αν θέλετε να αλλάξετε κάποιες ρυθμίσεις.
ReadyLabel2b=Πατήστε Εγκατάσταση για να συνεχίσετε.
ReadyMemoUserInfo=Πληροφορίες Χρήστη:
ReadyMemoDir=Κατάλογος προορισμού:
ReadyMemoType=Είδος εγκατάστασης:
ReadyMemoComponents=Επιλεγμένα συστατικά:
ReadyMemoGroup=Κατάλογος στο μενού Προγράμματα:
ReadyMemoTasks=Επιπλέον Ενέργειες:

; *** "Preparing to Install" wizard page
WizardPreparing=Προετοιμασία Εγκατάστασης
PreparingDesc=Η εγκατάσταση προετοιμάζει το πρόγραμμα [name] να τοποθετηθεί στον υπολογιστή.
PreviousInstallNotCompleted=The installation/removal of a previous program was not completed. You will need to restart your computer to complete that installation.%n%nAfter restarting your computer, run Setup again to complete the installation of [name].
CannotContinue=Setup cannot continue. Please click Cancel to exit.

; *** "Installing" wizard page
WizardInstalling=Πρόοδος Εγκατάστασης
InstallingLabel=Παρακαλώ περιμένετε να ολοκληρωθεί η εγκατάσταση του [name] στον υπολογιστή σας.

; *** "Setup Completed" wizard page
; 2.0.x
;WizardFinished=Η Εγκατάσταση Ολοκληρώθηκε
FinishedHeadingLabel=Completing the [name] Setup Wizard
FinishedLabelNoIcons=Η εγκατάσταση του [name] στον υπολογιστή σας τελείωσε με επιτυχία.
FinishedLabel=Η εγκατάσταση του [name] στον υπολογιστή σας τελείωσε με επιτυχία. Μπορείτε να ξεκινήσετε το πρόγραμμα επιλέγοντας το εικονίδιο που δημιουργήθηκε στο μενού εκκίνηση.
ClickFinish=Πατήστε Τέλος για να τερματίσετε το πρόγραμμα εγκατάστασης.
FinishedRestartLabel=Για να ολοκληρωθεί η εγκατάσταση του  [name] πρέπει να γίνει επανεκκίνηση του υπολογιστή σας. Θέλετε να γίνει τώρα;
FinishedRestartMessage=Για να ολοκληρωθεί η εγκατάσταση του  [name] πρέπει να γίνει επανεκκίνηση του υπολογιστή σας.%n%nΘέλετε να γίνει τώρα;
ShowReadmeCheck=Ναι θέλω να διαβάσω τις πληροφορίες του προγράμματος
YesRadio=&Ναι να γίνει επανεκκίνηση τώρα.
NoRadio=&Οχι θα κάνω επανεκκίνηση αργότερα.
; used for example as 'Run MyProg.exe'
RunEntryExec=Να εκτελεστεί το πρόγραμμα %1
; used for example as 'View Readme.txt'
RunEntryShellExec=Να εκτελεστεί το %1

; *** "Setup Needs the Next Disk" stuff
ChangeDiskTitle=Τοποθετήστε την επόμενη δισκέττα
; 4.0.x
;SelectDirectory=Επιλέξτε κατάλογο
SelectDiskLabel2=Τοποθετήστε την δισκέττα %1 και πατήστε Εντάξει.
PathLabel=&Διαδρομή
FileNotInDir2=Το αρχείο "%1" δεν βρίσκεται στο "%2". Τοποθετήστε τη σωστή δισκέττα.
SelectDirectoryLabel=Δώστε την τοποθεσία της επόμενης δισκέττας.

; *** Installation phase messages
SetupAborted=Η εγκατάσταση δεν ολοκληρώθηκε.%n%nΔιορθώστε το πρόβλημα και εκτελέστε ξανά την εγκατάσταση.
EntryAbortRetryIgnore=Πατήστε Retry για να ξαναπροσπαθήσετε,  Ignore για να συνεχίσετε η Abort για να τερματίσετε την εγκατάσταση.

; *** Installation status messages
StatusCreateDirs=Δημιουργία καταλόγων...
StatusExtractFiles=Αποσυμπίεση αρχείων...
StatusCreateIcons=Δημιουργία εικονιδίων...
StatusCreateIniEntries=Καταχώρηση στο ΙΝΙ αρχείο συστήματος...
StatusCreateRegistryEntries=Καταχώρηση στο μητρώο συστήματος...
StatusRegisterFiles=Καταχώρηση αρχείων
StatusSavingUninstall=Πληροφορίες απεγκατάστασης...
StatusRunProgram=Τελειώνοντας την εγκατάσταση...
StatusRollback=Rolling back changes...

; *** Misc. errors
; 2.0.x
;ErrorInternal=Σφάλμα %1
ErrorInternal2=Σφάλμα %1
ErrorFunctionFailedNoCode=%1 Σφάλμα
ErrorFunctionFailed=%1 Σφάλμα, κωδ. %2
ErrorFunctionFailedWithMessage=%1 Σφάλμα, κωδ. %2%n%3
ErrorExecutingProgram=Δεν μπορεί να εκτελεστεί το αρχείο:%n%1

;2.0.x
;ErrorDDEExecute=DDE: Σφάλμα κατά την εκτέλεση της ενέργειας (code: %1)
;ErrorDDECommandFailed=DDE: Η εντολή απέτυχε.
;ErrorDDERequest=DDE: Σφάλμα κατά την εκτέλεση της ενέργειας (code: %1)

; *** Registry errors
ErrorRegOpenKey=Δεν μπορεί να διαβαστεί το κλειδί μητρώου συστήματος:%n%1\%2
ErrorRegCreateKey=Δεν μπορεί να δημιουργηθεί το κλειδί μητρώου συστήματος:%n%1\%2
ErrorRegWriteKey=Δεν μπορεί να γίνει καταχώρηση στο κλειδί μητρώου συστήματος:%n%1\%2

; *** INI errors
ErrorIniEntry=Δεν μπορεί να γίνει καταχώρηση στο ΙΝΙ αρχείο συστήματος "%1".

; *** File copying errors
FileAbortRetryIgnore=Πατήστε Retry για να ξαναπροσπαθήσετε,  Ignore για να συνεχίσετε η Abort για να τερματίσετε την εγκατάσταση.
FileAbortRetryIgnore2=Πατήστε Retry για να ξαναπροσπαθήσετε,  Ignore για να συνεχίσετε η Abort για να τερματίσετε την εγκατάσταση.
SourceIsCorrupted=Το αρχείο προέλευσης είναι κατεστραμμένο.
SourceDoesntExist=Το αρχείο προέλευσης "%1" δεν υπάρχει.
ExistingFileReadOnly=Το αρχείο είναι μπαρκαρισμένο μόνο για ανάγνωση.%n%nΠατήστε Retry για να το ξεμαρκάρετε και να προσπαθήσετε πάλι, Ignore για να το προσπεράσετε η Abort για να τερματίσετε την εγκατάσταση.
ErrorReadingExistingDest=Παρουσιάστηκε σφάλμα κατά την ανάγνωση του αρχείου:
FileExists=Το αρχείο υπάρχει.%n%nΘέλετε να ξαναγραφτεί;
ExistingFileNewer=Ενα αρχείο που βρέθηκε στον υπολογιστή σας είναι νεότερης έκδοσης απο εκείνο της εγκατάστασης. Προτείνεται να κρατήσετε το υπάρχον αρχείο.%n%nΘέλετε να κρατήσετε το υπάρχον αρχείο;
ErrorChangingAttr=Προέκυψε σφάλμα στην προσπάθεια να αλλαχτούν τα χαρακτηριστικά του αρχείου:
ErrorCreatingTemp=Προέκυψε σφάλμα στην προσπάθεια να δημιουργηθεί ένα αρχείο στον κατάλογο προορισμού:
ErrorReadingSource=Προέκυψε σφάλμα στην προσπάθεια ανάγνωσης του αρχείου προέλευσης:
ErrorCopying=Προέκυψε σφάλμα στην προσπάθεια να αντιγραφεί το αρχείο:
ErrorReplacingExistingFile=Προέκυψε σφάλμα στην προσπάθεια να αντικατασταθεί το υπάρχον αρχείο:
ErrorRestartReplace=Προέκυψε σφάλμα στην προσπάθεια να γίνει επανεκκίνηση και αντικατάσταση αρχείου:
ErrorRenamingTemp=Προέκυψε σφάλμα στην προσπάθεια μετονομασίας ενός αρχείου στον κατάλογο προορισμού:
ErrorRegisterServer=Προέκυψε σφάλμα στην προσπάθεια καταχώρησης DLL/OCX: %1
ErrorRegSvr32Failed=RegSvr32 failed with exit code %1
ErrorRegisterTypeLib=Unable to register the type library: %1

; *** Post-installation errors
ErrorOpeningReadme=Προέκυψε σφάλμα στην προσπάθεια να φορτωθεί το αρχείο πληροφοριών.
ErrorRestartingComputer=Προέκυψε σφάλμα στην προσπάθεια επανεκκίνησης του υπολογιστή.%nΠαρακαλώ επανεκκινήστε τον υπολογιστή σας.

; *** Uninstaller messages
UninstallNotFound=Το αρχείο "%1" δεν βρέθηκε. Η απεγκατάσταση δεν μπορεί να γίνει
; 4.0.x
UninstallOpenError=Το αρχείο "%1" δεν μπόρεσε να φορτωθεί. Η απεγκατάσταση δεν μπορεί να γίνει
UninstallUnsupportedVer=Το αρχείο "%1" δεν αναγνωρίζεται από αυτή την έκδοση της εγκατάστασης, Η απεγκατάσταση δεν μπορεί να εκτελεστεί
UninstallUnknownEntry=Το αρχείο "%1" δεν αναγνωρίζεται από αυτή την έκδοση της εγκατάστασης, Η απεγκατάσταση δεν μπορεί να εκτελεστεί
ConfirmUninstall=Είστε σίγουροι ότι θέλετε να διαγράψετε το %1 και όλα τα συστατικά του;
; 5.1.0+
UninstallOnlyOnWin64=Αυτή η εφαρμογή μπορεί να απεγκατασταθεί μόνο σε 64-bit Windows.
OnlyAdminCanUninstall=Η απεγκατάσταση μπορεί να εκτελεστεί μόνο από τον Διαχειριστή συστήματος
UninstallStatusLabel=Παρακαλώ περιμένετε όσο το %1 διαγράφετε από τον υπολογιστή σας
UninstalledAll=Η απεγκατάσταση του %1 έγινε με επιτυχία.
UninstalledMost=Η απεγκατάσταση του %1 έγινε με επιτυχία.%n%nΚάποια συστατικά που παρέμειναν στον υπολογιστή σας θα πρέπει να τα διαγράψετε εσείς.
UninstalledAndNeedsRestart=To complete the uninstallation of %1, your computer must be restarted.%n%nWould you like to restart now?
UninstallDataCorrupted="%1" Αυτό το αρχείο είναι κατεστραμμένο. Δεν μπορεί να γίνει απεγκατάσταση.

; *** Uninstallation phase messages
ConfirmDeleteSharedFileTitle=Θέλετε να διαγραφούν τα κοινά αρχεία;
ConfirmDeleteSharedFile2=Τα κοινά αρχεία δεν χρησιμοποιούνται από κάποιο πρόγραμμα. Θέλετε να διαγραφούν;%n%nΑν κάποιο πρόγραμμα τα χρησιμοποιεί ίσως δεν εκτελείται σωστά αν τα διαγράψετε. Αν δεν είστε σίγουροι αφήστε τα στο σύστημα σας δεν προκαλούν κάποιο πρόβλημα.
SharedFileNameLabel=Ονομα Αρχείου:
SharedFileLocationLabel=Τοποθεσία:
WizardUninstalling=Πρόοδος Απεγκατάστασης:
StatusUninstalling=Απεγκατάσταση του %1...

[CustomMessages]
NameAndVersion=%1 έκδοση %2
AdditionalIcons=Επιπλέον εικονίδια:
CreateDesktopIcon=Δημιουργία ενός &εικονιδίου στην επιφάνεια εργασίας
CreateQuickLaunchIcon=Δημιουργία ενός εικονιδίου στη &Γρήγορη Εκκίνηση
ProgramOnTheWeb=Το %1 στο Internet
UninstallProgram=Απεγκατάσταση του %1
LaunchProgram=Εκκίνηση του %1
AssocFileExtension=%Αντιστοίχιση του %1 με την %2 επέκταση αρχείου
AssocingFileExtension=Γίνεται αντιστοίχηση του %1 με την %2 επέκταση αρχείου...

