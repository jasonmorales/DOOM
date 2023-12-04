    auto L = "asdf";

    string_view NSV = L;
    string NS;
    const string CNS = "CNS";

    std::string_view SV = L;
    std::string S;
    const std::string CS = "CS";

    NS = L;
    NS = NS;
    NS = NSV;
    NS = CNS;
    NS = S;
    NS = SV;
    NS = CS;

    NSV = L;
    NSV = NSV;
    NSV = CNS;
    NSV = S;
    NSV = SV;
    NSV = CS;

    S = L;
    S = NS;
    S = NSV;
    S = CNS;
    S = S;
    S = SV;
    S = CS;

    SV = L;
    SV = NSV;
    SV = CNS;
    SV = S;
    SV = SV;
    SV = CS;

    string_view sv1(L);
    string_view sv2(NSV);
    string_view sv3(NS);
    string_view sv4(SV);
    string_view sv5(S);
    string_view sv6("sv6");

    string s1(L);
    string s2(NSV);
    string s3(NS);
    string s4(SV);
    string s5(S);
    string s6("s6");

    std::string_view ssv1(L);
    std::string_view ssv2(NSV);
    std::string_view ssv3(NS);
    std::string_view ssv4(SV);
    std::string_view ssv5(S);
    std::string_view ssv6("ssv6");

    std::string ss1(L);
    std::string ss2(NSV);
    std::string ss3(NS);
    std::string ss4(SV);
    std::string ss5(S);
    std::string ss6("ss6");

