using GodotTools.Core;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;

namespace GodotTools.ProjectEditor
{
    public class DotNetSolution
    {


        private string directoryPath;
        private readonly Dictionary<string, ProjectInfo> projects = new Dictionary<string, ProjectInfo>();

        public string Name { get; }

        public string DirectoryPath
        {
            get => directoryPath;
            set => directoryPath = value.IsAbsolutePath() ? value : Path.GetFullPath(value);
        }

        public class ProjectInfo
        {
            public string Guid;
            public string PathRelativeToSolution;
            public List<string> Configs = new List<string>();
        }

        public void AddNewProject(string name, ProjectInfo projectInfo)
        {
            projects[name] = projectInfo;
        }

        public bool HasProject(string name)
        {
            return projects.ContainsKey(name);
        }

        public ProjectInfo GetProjectInfo(string name)
        {
            return projects[name];
        }

        public bool RemoveProject(string name)
        {
            return projects.Remove(name);
        }

        public void Save()
        {
            if (!Directory.Exists(DirectoryPath))
                throw new FileNotFoundException("The solution directory does not exist.");

            string projectsDecl = string.Empty;
            string slnPlatformsCfg = string.Empty;
            string projPlatformsCfg = string.Empty;

            bool isFirstProject = true;

            foreach (var pair in projects)
            {
                string name = pair.Key;
                ProjectInfo projectInfo = pair.Value;

                if (!isFirstProject)
                    projectsDecl += "\n";

                projectsDecl += string.Format(ProjectDeclaration,
                    name, projectInfo.PathRelativeToSolution.Replace("/", "\\"), projectInfo.Guid);

                for (int i = 0; i < projectInfo.Configs.Count; i++)
                {
                    string config = projectInfo.Configs[i];

                    if (i != 0 || !isFirstProject)
                    {
                        slnPlatformsCfg += "\n";
                        projPlatformsCfg += "\n";
                    }

                    slnPlatformsCfg += string.Format(SolutionPlatformsConfig, config);
                    projPlatformsCfg += string.Format(ProjectPlatformsConfig, projectInfo.Guid, config);
                }

                isFirstProject = false;
            }

            string solutionPath = Path.Combine(DirectoryPath, Name + ".sln");
            string content = string.Format(SolutionTemplate, projectsDecl, slnPlatformsCfg, projPlatformsCfg);

            File.WriteAllText(solutionPath, content, Encoding.UTF8); // UTF-8 with BOM
        }

        public DotNetSolution(string name)
        {
            Name = name;
        }

        const string SolutionTemplate =
@"Microsoft Visual Studio Solution File, Format Version 12.00
# Visual Studio 2012
{0}
Global
	GlobalSection(SolutionConfigurationPlatforms) = preSolution
{1}
	EndGlobalSection
	GlobalSection(ProjectConfigurationPlatforms) = postSolution
{2}
	EndGlobalSection
EndGlobal
";

        const string ProjectDeclaration =
@"Project(""{{FAE04EC0-301F-11D3-BF4B-00C04F79EFBC}}"") = ""{0}"", ""{1}"", ""{{{2}}}""
EndProject";

        const string SolutionPlatformsConfig =
@"	{0}|Any CPU = {0}|Any CPU";
        const string ProjectPlatformsConfig =
@"		{{{0}}}.{1}|Any CPU.ActiveCfg = {1}|Any CPU
		{{{0}}}.{1}|Any CPU.Build.0 = {1}|Any CPU";
    }
}
