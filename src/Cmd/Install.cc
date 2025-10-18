#include "Install.hpp"

#include "Algos.hpp"
#include "BuildConfig.hpp"
#include "Builder/BuildProfile.hpp"
#include "Cli.hpp"
#include "Cmd/Common.hpp"
#include "Command.hpp"
#include "Diag.hpp"
#include "Manifest.hpp"

#include <fmt/format.h>
#include <fstream>
#include <string>

namespace cabin {

static Result<void> installMain(CliArgsView args);

const Subcmd INSTALL_CMD = //
    Subcmd{ "install" }
        .setDesc("Install the built binary and libraries")
        .addOpt(OPT_RELEASE)
        .addOpt(Opt{ "--prefix" }
                    .setDesc("Install destination prefix (default: /usr/local)")
                    .setPlaceholder("<PREFIX>"))
        .addOpt(
            Opt{ "--libdir" }
                .setDesc("Library directory name under prefix (default: lib)")
                .setPlaceholder("<LIBDIR>"))
        .setMainFn(installMain);

static Result<void> installMain(const CliArgsView args) {
  // Parse args
  BuildProfile buildProfile = BuildProfile::Dev;
  std::string prefix; // empty => use Makefile default
  std::string libdir; // empty => use Makefile default

  for (auto itr = args.begin(); itr != args.end(); ++itr) {
    const std::string_view arg = *itr;

    const auto control = Try(Cli::handleGlobalOpts(itr, args.end(), "install"));
    if (control == Cli::Return) {
      return Ok();
    } else if (control == Cli::Continue) {
      continue;
    } else if (arg == "-r" || arg == "--release") {
      buildProfile = BuildProfile::Release;
    } else if (arg == "--prefix") {
      if (itr + 1 == args.end()) {
        return Subcmd::missingOptArgumentFor(arg);
      }
      prefix = *++itr;
    } else if (arg == "--libdir") {
      if (itr + 1 == args.end()) {
        return Subcmd::missingOptArgumentFor(arg);
      }
      libdir = *++itr;
    } else {
      return INSTALL_CMD.noSuchArg(arg);
    }
  }

  const Manifest manifest = Try(Manifest::tryParse());

  // Generate or update Makefile; ensure it contains the install target
  BuildConfig config =
      Try(emitMakefile(manifest, buildProfile, /*includeDevDeps=*/false));
  // Ensure the target exists even if Makefile timestamp was considered
  // up-to-date
  Try(config.configureBuild());
  {
    std::ofstream makefile(config.outBasePath / "Makefile");
    Try(config.emitMakefile(makefile));
  }

  Ensure(config.hasBinTarget() || config.hasLibTarget(),
         "install requires a binary or library target in the project");

  // Run make install with optional PREFIX override
  const std::string outDir = config.outBasePath;
  Command makeCmd =
      getMakeCommand().addArg("-C").addArg(outDir).addArg("install");
  if (!prefix.empty()) {
    makeCmd.addArg(fmt::format("PREFIX={}", prefix));
  }
  if (!libdir.empty()) {
    makeCmd.addArg(fmt::format("LIBDIR={}", libdir));
  }

  Diag::info("Installing", "{} v{} ({})", manifest.package.name,
             manifest.package.version.toString(),
             manifest.path.parent_path().string());

  const ExitStatus status = Try(execCmd(makeCmd));
  Ensure(status.success(), "install {}", status);
  return Ok();
}

} // namespace cabin
