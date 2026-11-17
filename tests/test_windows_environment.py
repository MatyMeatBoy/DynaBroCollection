import unittest
from tools.windows_environment import portable_windows_env


class WindowsEnvironmentTests(unittest.TestCase):
    def test_accepts_windows_case_variants_without_retaining_development_paths(self):
        for spelling in ['SystemRoot','SYSTEMROOT','systemroot']:
            with self.subTest(spelling=spelling):
                source={spelling:r'C:\Windows','Path':'development-dlls','PATH':'other-dlls','KEEP':'value'}
                result=portable_windows_env(source)
                self.assertEqual(result['PATH'],r'C:\Windows\System32;C:\Windows')
                self.assertEqual([key for key in result if key.casefold()=='path'],['PATH'])
                self.assertEqual(result['KEEP'],'value')
                self.assertEqual(source['Path'],'development-dlls')

    def test_rejects_missing_or_relative_windows_directory(self):
        for source in [{},{'SYSTEMROOT':'relative'}]:
            with self.assertRaises(ValueError):portable_windows_env(source)
