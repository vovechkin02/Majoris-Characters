/** Test-only bridge to the existing C assertion suites. */
[CCode (cheader_filename = "search-contract-tests.h")]
namespace Characters.TestContracts {
    /** Execute the C fixture suite using the linked Vala factories. */
    [CCode (cname = "characters_run_contract_tests")]
    public static int run ();
}
