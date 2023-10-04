#tool nuget:?package=NuGet.CommandLine&version=5.10

var target = Argument("target", "Default");


var ridX64 = "win10-x64";
var ridArm64 = "win10-arm64";
var frameworkVersion = "net8.0-windows10.0.20348.0";


//var platformTargets = new[]{PlatformTarget.x64, PlatformTarget.ARM64};
var platformTargets = new[]{PlatformTarget.x64};



// we don't actually support debug builds for this. 
var configuration = "Release";

var repoRootDir = MakeAbsolute(Directory("../")).ToString();

var srcDir = System.IO.Path.Combine(repoRootDir, "src");
var buildRootDir = System.IO.Path.Combine(repoRootDir, "build");

var stagingRootDir = System.IO.Path.Combine(buildRootDir, "staging");
var releaseRootDir = System.IO.Path.Combine(buildRootDir, "release");

var stagingX64Dir = System.IO.Path.Combine(stagingRootDir, ridX64);
var stagingArm64Dir = System.IO.Path.Combine(stagingRootDir, ridArm64);

string stagingDir;

//////////////////////////////////////////////////////////////////////
// PROJECT LOCATIONS
//////////////////////////////////////////////////////////////////////
///

var registrySettingsStagingDir = System.IO.Path.Combine(stagingRootDir, "reg");

var apiAndServiceSolutionDir = System.IO.Path.Combine(srcDir, "api");
var apiAndServiceSolutionFile = System.IO.Path.Combine(apiAndServiceSolutionDir, "Midi2.sln");
var apiAndServiceStagingDir = System.IO.Path.Combine(stagingRootDir, "api");

var sdkSolutionDir = System.IO.Path.Combine(srcDir, "app-dev-sdk");
var sdkSolutionFile = System.IO.Path.Combine(sdkSolutionDir, "midi-sdk.sln");
var sdkStagingDir = System.IO.Path.Combine(stagingRootDir, "app-dev-sdk");

var vsFilesDir = System.IO.Path.Combine(apiAndServiceSolutionDir, "VSFiles");

var consoleAppSolutionDir = System.IO.Path.Combine(srcDir, "user-tools", "midi-console");
var consoleAppSolutionFile = System.IO.Path.Combine(consoleAppSolutionDir, "midi-console.sln");
var consoleAppProjectDir = System.IO.Path.Combine(consoleAppSolutionDir, "Midi");
var consoleAppProjectFile = System.IO.Path.Combine(consoleAppProjectDir, "Midi.csproj");
var consoleAppStagingDir = System.IO.Path.Combine(stagingRootDir, "midi-console");

var settingsAppSolutionDir = System.IO.Path.Combine(srcDir, "user-tools", "midi-settings");
var settingsAppSolutionFile = System.IO.Path.Combine(settingsAppSolutionDir, "midi-settings.sln");

var settingsAppProjectDir = System.IO.Path.Combine(settingsAppSolutionDir, "Microsoft.Midi.Settings");
var settingsAppProjectFile = System.IO.Path.Combine(settingsAppProjectDir, "Microsoft.Midi.Settings.csproj");

var settingsAppStagingDir = System.IO.Path.Combine(stagingRootDir, "midi-settings");


var setupSolutionDir = System.IO.Path.Combine(srcDir, "oob-setup");
var setupSolutionFile = System.IO.Path.Combine(setupSolutionDir, "midi-services-setup.sln");

var setupReleaseDir = releaseRootDir;



// we always want the latest redist. These are permalinks.
var vcRedistArm64URL = new Uri("https://aka.ms/vs/17/release/vc_redist.arm64.exe");
var vcRedistX64URL = new Uri("https://aka.ms/vs/17/release/vc_redist.x64.exe");

var vcRedistDir = "";

var allowPreviewVersionOfBuildTools = true;

//////////////////////////////////////////////////////////////////////
// TASKS
//////////////////////////////////////////////////////////////////////

Task("Clean")
    .WithCriteria(c => HasArgument("rebuild"))
    .Does(() =>
{
    CleanDirectory(stagingRootDir);
    CleanDirectory(vsFilesDir);

    //CleanDirectory(buildX64Dir);
    //CleanDirectory(buildArm64Dir);
});


Task("VerifyDependencies")
    .Does(() =>
{
    // Grab latest VC runtime

    // https://aka.ms/vs/17/release/vc_redist.x64.exe


    // Grab lates t.NET runtime if the file isn't there. It will have a long name with a 
    // version number, like dotnet-runtime-8.0.0-rc.1.23419.4-win-x64 so you'llneed to 
    // rename that for the installer
    // https://aka.ms/dotnet/8.0/preview/windowsdesktop-runtime-win-x64.exe



});

Task("SetupEnvironment")
    .Does(()=>
{
    // TODO: Need to verify that %MIDI_REPO_ROOT% is set. If not, set it to the root \midi folder
    var rootVariableExists = !string.IsNullOrWhiteSpace(Environment.GetEnvironmentVariable("MIDI_REPO_ROOT"));

    if (!rootVariableExists)
    {
        // this will only work if this folder is located in \midi\build or some other subfolder of the root \midi folder
        Environment.SetEnvironmentVariable("MIDI_REPO_ROOT", System.IO.Path.GetFullPath("..\\"));
    }
});


 Task("BuildServiceAndAPI")
    .DoesForEach(platformTargets, plat =>
{
    Information("\nBuilding service and API for " + plat.ToString());

    string abstractionRootDir = System.IO.Path.Combine(apiAndServiceSolutionDir, "Abstraction");

    // the projects, as configured for internal Razzle build, rely on this folder

    string workingDirectory = string.Empty;

    var buildSettings = new MSBuildSettings
    {
        MaxCpuCount = 0,
        Configuration = configuration,
        AllowPreviewVersion = allowPreviewVersionOfBuildTools,
        PlatformTarget = plat,
        Verbosity = Verbosity.Minimal,
    };

    MSBuild(System.IO.Path.Combine(apiAndServiceSolutionDir, "Midi2.sln"), buildSettings);

    // Copy key output files from VSFiles to staging to allow building installer
   
    var outputDir = System.IO.Path.Combine(apiAndServiceSolutionDir, "VSFiles", plat.ToString(), configuration);

    Information("\nCopying service and API for " + plat.ToString());

    var copyToDir = System.IO.Path.Combine(apiAndServiceStagingDir, plat.ToString());
    
    if (!DirectoryExists(copyToDir))
        CreateDirectory(copyToDir);

    // copy all the DLLs
    CopyFiles(System.IO.Path.Combine(outputDir, "*.dll"), copyToDir); 

    CopyFiles(System.IO.Path.Combine(outputDir, "*.pri"), copyToDir); 
    CopyFiles(System.IO.Path.Combine(outputDir, "*.pdb"), copyToDir); 

    CopyFiles(System.IO.Path.Combine(outputDir, "MidiSrv.exe"), copyToDir); 

    CopyFiles(System.IO.Path.Combine(outputDir, "Windows.Devices.Midi2.winmd"), copyToDir); 

    CopyFiles(System.IO.Path.Combine(outputDir, "WinRTActivationEntries.txt"), copyToDir); 
});


Task("BuildApiActivationRegEntries")
    .IsDependentOn("BuildServiceAndAPI")
    .DoesForEach(platformTargets, plat =>
{
    Information("\nBuilding WinRT Activation Entries for " + plat.ToString());

    // read the file of dependencies
    var sourceFileName = System.IO.Path.Combine(apiAndServiceStagingDir, plat.ToString(), "WinRTActivationEntries.txt");
    var wxiDestinationFileName = System.IO.Path.Combine(apiAndServiceStagingDir, plat.ToString(), "WinRTActivationEntries.wxi");

    const string parentHKLMRegKey = "SOFTWARE\\Microsoft\\WindowsRuntime\\ActivatableClassId\\";
    const string wixWinrtLibFileName = "[API_INSTALLFOLDER]Windows.Devices.Midi2.dll";

    if (!System.IO.File.Exists(sourceFileName))
        throw new ArgumentException("Missing WinRT Activation entries file " + sourceFileName);

    using (StreamReader reader = System.IO.File.OpenText(sourceFileName))
    {
        using (StreamWriter wxiWriter = System.IO.File.CreateText(wxiDestinationFileName))            
        {
            wxiWriter.WriteLine("<Include xmlns=\"http://wixtoolset.org/schemas/v4/wxs\">");

            string line;

            while ((line = reader.ReadLine()) != null)
            {
                string trimmedLine = line.Trim();

                if (string.IsNullOrWhiteSpace(trimmedLine) || trimmedLine.StartsWith("#"))
                {
                    // comment or empty line
                    continue;
                }

                // pipe-delimited lines
                var elements = trimmedLine.Split('|');

                if (elements.Count() != 4)
                    throw new ArgumentException("Bad line:  " + trimmedLine); 

                // entries in order are
                // ClassName | ActivationType | Threading | TrustLevel

                string className = elements[0].Trim();
                string activationType = elements[1].Trim();
                string threading = elements[2].Trim();
                string trustLevel = elements[3].Trim();

                wxiWriter.WriteLine($"<RegistryKey Root=\"HKLM\" Key=\"{parentHKLMRegKey}{className}\">");
                
                wxiWriter.WriteLine($"    <RegistryValue Name=\"DllPath\" Type=\"string\" Value=\"{wixWinrtLibFileName}\" />");
                wxiWriter.WriteLine($"    <RegistryValue Name=\"ActivationType\" Type=\"integer\" Value=\"{activationType}\" />");
                wxiWriter.WriteLine($"    <RegistryValue Name=\"Threading\" Type=\"integer\" Value=\"{threading}\" />");
                wxiWriter.WriteLine($"    <RegistryValue Name=\"TrustLevel\" Type=\"integer\" Value=\"{trustLevel}\" />");
               
                wxiWriter.WriteLine("</RegistryKey>");

            }

            wxiWriter.WriteLine("</Include>");

        }
    }

});


Task("BuildApiActivationRegEntriesCSharp")
    .IsDependentOn("BuildServiceAndAPI")
    .DoesForEach(platformTargets, plat =>
{
    Information("\nBuilding WinRT Activation Entries for " + plat.ToString());

    // read the file of dependencies
    var sourceFileName = System.IO.Path.Combine(apiAndServiceStagingDir, plat.ToString(), "WinRTActivationEntries.txt");
    var csDestinationFileName = System.IO.Path.Combine(registrySettingsStagingDir, "WinRTActivationEntries.cs");

//    const string parentHKLMRegKey = "SOFTWARE\\Microsoft\\WindowsRuntime\\ActivatableClassId\\";
//    const string wixWinrtLibFileName = "[API_INSTALLFOLDER]Windows.Devices.Midi2.dll";

    if (!System.IO.File.Exists(sourceFileName))
        throw new ArgumentException("Missing WinRT Activation entries file " + sourceFileName);

    using (StreamReader reader = System.IO.File.OpenText(sourceFileName))
    {

        if (!DirectoryExists(registrySettingsStagingDir))
            CreateDirectory(registrySettingsStagingDir);

        using (StreamWriter csWriter = System.IO.File.CreateText(csDestinationFileName))            
        {
            csWriter.WriteLine("// This file was generated by the build process");
            csWriter.WriteLine("//");
            csWriter.WriteLine("using RegistryCustomActions;");
            csWriter.WriteLine("class RegistryEntries");
            csWriter.WriteLine("{");
            csWriter.WriteLine("    public RegEntry[] ActivationEntries = new RegEntry[]");
            csWriter.WriteLine("    {");

            string line;

            while ((line = reader.ReadLine()) != null)
            {
                string trimmedLine = line.Trim();

                if (string.IsNullOrWhiteSpace(trimmedLine) || trimmedLine.StartsWith("#"))
                {
                    // comment or empty line
                    continue;
                }

                // pipe-delimited lines
                var elements = trimmedLine.Split('|');

                if (elements.Count() != 4)
                    throw new ArgumentException("Bad line:  " + trimmedLine); 

                // entries in order are
                // ClassName | ActivationType | Threading | TrustLevel

                string className = elements[0].Trim();
                string activationType = elements[1].Trim();
                string threading = elements[2].Trim();
                string trustLevel = elements[3].Trim();

                csWriter.WriteLine($"        new RegEntry{{ ClassName=\"{className}\", ActivationType={activationType}, Threading={threading}, TrustLevel={trustLevel}  }},");


            }

            csWriter.WriteLine("    };");
            csWriter.WriteLine("}");


        }
    }

});



Task("PackAPIProjection")
    .IsDependentOn("BuildServiceAndAPI")
    .Does(() =>
{
    Information("\nPacking API Projection...");

    var workingDirectory = System.IO.Path.Combine(apiAndServiceSolutionDir, "Client", "Midi2Client-Projection");

    NuGetPack(System.IO.Path.Combine(workingDirectory, "nuget", "Windows.Devices.Midi2.nuspec"), new NuGetPackSettings
    {
        WorkingDirectory = workingDirectory,
        OutputDirectory = System.IO.Path.Combine(releaseRootDir, "NuGet")  // NuGet packages cover multiple platforms
    });


});



Task("BuildSDK")
    .IsDependentOn("BuildServiceAndAPI")
    .IsDependentOn("PackAPIProjection")
    .DoesForEach(platformTargets, plat => 
{
    Information("\nBuilding SDK for " + plat.ToString());

    // TODO


});


Task("PackSDKProjection")
    .IsDependentOn("BuildSDK")
    .Does(() => 
{
    Information("\nPacking SDK...");

    // TODO


});



Task("BuildConsoleApp")
    .IsDependentOn("PackAPIProjection")
    .IsDependentOn("PackSDKProjection")
    .DoesForEach(platformTargets, plat =>
{
    // TODO: Update nuget ref in console app to the new version
   

    Information("\nBuilding MIDI console app for " + plat.ToString());

    // update nuget packages for the entire solution. This is important for API/SDK NuGet in particular

    NuGetUpdate(consoleAppSolutionFile, new NuGetUpdateSettings
    {
        WorkingDirectory = consoleAppSolutionDir,
    });

    // we're specifying a rid, so we need to compile the project, not the solution
    
    string rid = "";
    if (plat == PlatformTarget.x64)
        rid = ridX64;
    else if (plat == PlatformTarget.ARM64)
        rid = ridArm64;
    else
        throw new ArgumentException("Invalid platform target " + plat.ToString());

    DotNetBuild(consoleAppProjectFile, new DotNetBuildSettings
    {
        WorkingDirectory = consoleAppProjectDir,
        Configuration = configuration, 
        Runtime = rid,        
    });


    // Note: Spectre.Console doesn't support trimming, so you get a huge exe.
    // Consider making this framework-dependent once .NET 8 releases
    DotNetPublish(consoleAppProjectFile, new DotNetPublishSettings
    {
        WorkingDirectory = consoleAppProjectDir,
        OutputDirectory = System.IO.Path.Combine(consoleAppStagingDir, plat.ToString()),
        Configuration = configuration,

        PublishSingleFile = false,
        PublishTrimmed = false,
        SelfContained = false,
        Framework = frameworkVersion,
        Runtime = rid
    });

});


Task("BuildSettingsApp")
    .IsDependentOn("PackAPIProjection")
    .IsDependentOn("PackSDKProjection")
    .DoesForEach(platformTargets, plat =>
{
    // TODO: Update nuget ref in settings app to the new version
    
    //NuGetUpdate(settingsAppSolutionFile);

    // TODO: Update nuget ref in console app to the new version
   

    Information("\nBuilding MIDI settings app for " + plat.ToString());

    // update nuget packages for the entire solution. This is important for API/SDK NuGet in particular

    //NuGetUpdate(settingsAppSolutionFile, new NuGetUpdateSettings
    //{
    //    WorkingDirectory = settingsAppSolutionDir,
    //});

    // we're specifying a rid, so we need to compile the project, not the solution
    
    string rid = "";
    if (plat == PlatformTarget.x64)
        rid = ridX64;
    else if (plat == PlatformTarget.ARM64)
        rid = ridArm64;
    else
        throw new ArgumentException("Invalid platform target " + plat.ToString());

    DotNetBuild(settingsAppProjectFile, new DotNetBuildSettings
    {
        WorkingDirectory = settingsAppSolutionDir,
        Configuration = configuration, 
        Runtime = rid,        
    });


    DotNetPublish(settingsAppProjectFile, new DotNetPublishSettings
    {
        WorkingDirectory = settingsAppSolutionDir,
        OutputDirectory = System.IO.Path.Combine(settingsAppStagingDir, plat.ToString()),
        Configuration = configuration,

        Runtime = rid,
        PublishSingleFile = false,
        PublishTrimmed = false,
        SelfContained = false,
        Framework = frameworkVersion
    });

    // WinUI makes me manually copy this over. If not for the hacked-in post-build step, would need all the .xbf files as well

});



Task("BuildInstaller")
    .IsDependentOn("SetupEnvironment")
    .IsDependentOn("BuildServiceAndAPI")
    .IsDependentOn("BuildApiActivationRegEntriesCSharp")
    .IsDependentOn("BuildSDK")
    .IsDependentOn("BuildSettingsApp")
    .IsDependentOn("BuildConsoleApp")
    .DoesForEach(platformTargets, plat => 
{
    /*var buildSettings = new MSBuildSettings
    {
        MaxCpuCount = 0,
        Configuration = configuration,
        AllowPreviewVersion = allowPreviewVersionOfBuildTools,
        PlatformTarget = plat,
        Verbosity = Verbosity.Minimal,       
    };

    MSBuild(setupSolutionFile, buildSettings); */

    // have to build these projects using dotnet build, or else the nuget references just die if you use MSBuild or VS

    //var apiProjectDir = System.IO.Path.Combine(setupSolutionDir, "api-package");
    //var apiProjectFile = System.IO.Path.Combine(apiProjectDir, "api-package.wixproj");

    var mainBundleProjectDir = System.IO.Path.Combine(setupSolutionDir, "main-bundle");
    //var mainBundleProjectFile = System.IO.Path.Combine(mainBundleProjectDir, "main-bundle.wixproj");

    //var regActionsProjectDir = System.IO.Path.Combine(setupSolutionDir, "RegistryCustomActions");
    //var regActionsProjectFile = System.IO.Path.Combine(regActionsProjectDir, "RegistryCustomActions.csproj");

    var buildSettings = new DotNetBuildSettings
    {
        WorkingDirectory = mainBundleProjectDir,
        Configuration = configuration, 
    };

            // if we don't set platform here, it always ends up as a 32 bit installer
            // configuration 
   // buildSettings.MSBuildSettings.Properties["Platform"] = plat;


    // build the custom action first
    DotNetBuild(setupSolutionFile, buildSettings);
    
    //CopyFiles(System.IO.Path.Combine(mainBundleProjectDir, "bin", "Release", "*.exe"), setupReleaseDir); 
    

    /*var msbuildSettings = new MSBuildSettings
    {
        MaxCpuCount = 0,
        Configuration = configuration,
        AllowPreviewVersion = allowPreviewVersionOfBuildTools,
        PlatformTarget = plat,
        Verbosity = Verbosity.Minimal,       
    };
w
    MSBuild(regActionsProjectFile, msbuildSettings);
    MSBuild(apiProjectFile, msbuildSettings);
    MSBuild(mainBundleProjectFile, msbuildSettings); */

    // built-in WiX support doesn't seem to work with WiX and burn bundles.

    /*var buildSettings = new DotNetBuildSettings
    {
        WorkingDirectory = regActionsProjectDir,
        Configuration = configuration, 
        Runtime = rid,
    };
    
    DotNetBuild(regActionsProjectFile, buildSettings); */


    if (!DirectoryExists(setupReleaseDir))
        CreateDirectory(setupReleaseDir);

    CopyFiles(System.IO.Path.Combine(mainBundleProjectDir, "bin", plat.ToString(), "Release", "*.exe"), setupReleaseDir); 
});




//////////////////////////////////////////////////////////////////////
// TASK TARGETS
//////////////////////////////////////////////////////////////////////

Task("Default")
    .IsDependentOn("Clean")
    .IsDependentOn("BuildInstaller");




//////////////////////////////////////////////////////////////////////
// EXECUTION
//////////////////////////////////////////////////////////////////////

RunTarget(target);