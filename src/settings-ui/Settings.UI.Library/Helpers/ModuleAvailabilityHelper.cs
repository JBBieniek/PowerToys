// Copyright (c) Microsoft Corporation
// The Microsoft Corporation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using ManagedCommon;
using Microsoft.Win32.SafeHandles;

namespace Microsoft.PowerToys.Settings.UI.Library.Helpers
{
    public static class ModuleAvailabilityHelper
    {
        public static bool IsModuleAvailable(ModuleType moduleType)
        {
            if (moduleType != ModuleType.GlideZoom)
            {
                return true;
            }

            // Capability feedback only. Windows still validates the executable's
            // signature, secure location and UIAccess token when it is launched.
            try
            {
                string[] roots =
                [
                    Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles),
                    Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86),
                    Environment.GetFolderPath(Environment.SpecialFolder.System),
                ];

                var directory = AppContext.BaseDirectory;
                for (var depth = 0; depth < 8 && IsSupportedGlideZoomInstallPath(directory, roots); depth++)
                {
                    var executable = Path.Combine(directory, "PowerToys.exe");
                    if (File.Exists(executable))
                    {
                        foreach (var root in roots)
                        {
                            if (IsSupportedGlideZoomInstallPath(directory, root) &&
                                TryGetFinalPath(root, true, out var finalRoot) &&
                                AreGlideZoomFilesAvailable(directory, root, finalRoot, path => TryGetFinalPath(path, false, out var resolved) ? resolved : null))
                            {
                                return true;
                            }
                        }

                        return false;
                    }

                    directory = Path.GetDirectoryName(directory.TrimEnd(Path.DirectorySeparatorChar));
                }
            }
            catch (Exception)
            {
                // Unknown paths or inaccessible files do not establish availability.
            }

            return false;
        }

        public static bool IsSupportedGlideZoomInstallation(string directory, string resolvedDirectory, string secureRoot, string resolvedSecureRoot)
        {
            return IsSupportedGlideZoomInstallPath(directory, secureRoot) && IsSupportedGlideZoomInstallPath(resolvedDirectory, resolvedSecureRoot);
        }

        public static bool AreGlideZoomFilesAvailable(string directory, string secureRoot, string resolvedSecureRoot, Func<string, string> resolveFile)
        {
            if (resolveFile == null || !IsSupportedGlideZoomInstallPath(directory, secureRoot))
            {
                return false;
            }

            string[] requiredFiles =
            [
                "PowerToys.exe",
                "PowerToys.GlideZoomModuleInterface.dll",
                "PowerToys.GlideZoom.exe",
            ];

            foreach (var file in requiredFiles)
            {
                var resolved = resolveFile(Path.Combine(directory, file));
                if (!TryNormalizeDirectory(resolved, out var finalPath) ||
                    !IsSupportedGlideZoomInstallation(directory, Path.GetDirectoryName(finalPath), secureRoot, resolvedSecureRoot))
                {
                    return false;
                }
            }

            return true;
        }

        // Match the native module's local, canonical directory-boundary policy.
        // The caller must check both the original and handle-resolved locations.
        public static bool IsSupportedGlideZoomInstallPath(string directory, params string[] secureRoots)
        {
            if (!TryNormalizeDirectory(directory, out var candidate) || secureRoots == null)
            {
                return false;
            }

            foreach (var root in secureRoots)
            {
                if (TryNormalizeDirectory(root, out var normalizedRoot) &&
                    candidate.StartsWith(normalizedRoot + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase))
                {
                    return true;
                }
            }

            return false;
        }

        private static bool TryNormalizeDirectory(string path, out string normalized)
        {
            normalized = string.Empty;
            if (string.IsNullOrEmpty(path) || path.Length < 4 || path.Length > 32767 ||
                !((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')) ||
                path[1] != ':' || path[2] != '\\')
            {
                return false;
            }

            var end = path.EndsWith(Path.DirectorySeparatorChar) ? path.Length - 1 : path.Length;
            var segmentStart = 3;
            for (var index = 3; index <= end; index++)
            {
                if (index == end || path[index] == '\\')
                {
                    if (index == segmentStart || path[index - 1] is '.' or ' ')
                    {
                        return false;
                    }

                    segmentStart = index + 1;
                }
                else if (path[index] < ' ' || path[index] is '<' or '>' or '"' or '|' or '?' or '*' or ':' or '/')
                {
                    return false;
                }
            }

            normalized = path[..end];
            return true;
        }

        private static bool TryGetFinalPath(string path, bool directory, out string resolved)
        {
            resolved = string.Empty;
            if (!TryNormalizeDirectory(path, out _))
            {
                return false;
            }

            // FILE_READ_ATTRIBUTES, shared read/write/delete, OPEN_EXISTING.
            using var handle = CreateFile(path, 0x80, 7, IntPtr.Zero, 3, directory ? 0x02000000u : 0x80u, IntPtr.Zero);
            if (handle.IsInvalid)
            {
                return false;
            }

            var finalPath = new StringBuilder(32768);
            var length = GetFinalPathNameByHandle(handle, finalPath, (uint)finalPath.Capacity, 0);
            if (length == 0 || length >= finalPath.Capacity)
            {
                return false;
            }

            var value = finalPath.ToString();
            if (value.StartsWith(@"\\?\", StringComparison.Ordinal))
            {
                value = value[4..];
            }

            return TryNormalizeDirectory(value, out resolved);
        }

        [DllImport("kernel32.dll", EntryPoint = "CreateFileW", CharSet = CharSet.Unicode, ExactSpelling = true, SetLastError = true)]
        [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
        private static extern SafeFileHandle CreateFile(string path, uint access, uint share, IntPtr security, uint creation, uint flags, IntPtr template);

        [DllImport("kernel32.dll", EntryPoint = "GetFinalPathNameByHandleW", CharSet = CharSet.Unicode, ExactSpelling = true, SetLastError = true)]
        [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
        private static extern uint GetFinalPathNameByHandle(SafeFileHandle file, StringBuilder path, uint capacity, uint flags);
    }
}
