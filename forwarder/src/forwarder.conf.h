static const char FORWARDER_CONF[] =
"\n"
"# Sample ICQ forwarder configuration file\n"
"# Edit and copy to ~/.licq\n"
"\n"
"# This is the type of forwarding desired:\n"
"# 0 - Email\n"
"# 1 - ICQ\n"
"\n"
"[Forward]\n"
"Type=0\n"
"\n"
"# These options are for forwarding to an email account:\n"
"#  Host - the smtp host to talk to\n"
"#  To - who to forward icq messages to\n"
"#  From - address used as return path\n"
"#  Domain - your local domain, \"localhost\" should work fine\n"
"\n"
"[SMTP]\n"
"Host=localhost\n"
"To=root@localhost\n"
"From=root@localhost\n"
"Domain=localhost\n"
"\n"
"# These options are for forwarding to an icq number:\n"
"#  Uin - the uin to forward to\n"
"\n"
"[ICQ]\n"
"Uin=0\n"
"\n";
