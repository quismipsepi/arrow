# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

require "English"
require "open-uri"
require "time"

class PackageTask
  include Rake::DSL

  def initialize(package, version, release_time, options={})
    @package = package
    @version = version
    @release_time = release_time

    @archive_base_name = "#{@package}-#{@version}"
    @archive_name = "#{@archive_base_name}.tar.gz"
    @full_archive_name = File.expand_path(@archive_name)

    @rpm_package = @package
    case @version
    when /-((dev|rc)\d+)\z/
      base_version = $PREMATCH
      sub_version = $1
      type = $2
      if type == "rc" and options[:rc_build_type] == :release
        @deb_upstream_version = base_version
        @deb_archive_base_name_version = base_version
        @rpm_version = base_version
        @rpm_release = "1"
      else
        @deb_upstream_version = "#{base_version}~#{sub_version}"
        @deb_archive_base_name_version = @version
        @rpm_version = base_version
        @rpm_release = "0.#{sub_version}"
      end
    else
      @deb_upstream_version = @version
      @deb_archive_base_name_version = @version
      @rpm_version = @version
      @rpm_release = "1"
    end
    @deb_release = "1"
  end

  def define
    define_dist_task
    define_apt_task
    define_yum_task
    define_version_task
    define_docker_tasks
  end

  private
  def env_value(name)
    value = ENV[name]
    raise "Specify #{name} environment variable" if value.nil?
    value
  end

  def debug_build?
    ENV["DEBUG"] != "no"
  end

  def git_directory?(directory)
    candidate_paths = [".git", "HEAD"]
    candidate_paths.any? do |candidate_path|
      File.exist?(File.join(directory, candidate_path))
    end
  end

  def latest_commit_time(git_directory)
    return nil unless git_directory?(git_directory)
    cd(git_directory) do
      return Time.iso8601(`git log -n 1 --format=%aI`.chomp).utc
    end
  end

  def download(url, output_path)
    if File.directory?(output_path)
      base_name = url.split("/").last
      output_path = File.join(output_path, base_name)
    end
    absolute_output_path = File.expand_path(output_path)

    unless File.exist?(absolute_output_path)
      mkdir_p(File.dirname(absolute_output_path))
      rake_output_message "Downloading... #{url}"
      URI(url).open do |downloaded_file|
        File.open(absolute_output_path, "wb") do |output_file|
          output_file.print(downloaded_file.read)
        end
      end
    end

    absolute_output_path
  end

  def substitute_content(content)
    content.gsub(/@(.+?)@/) do |matched|
      yield($1, matched)
    end
  end

  def docker_image(os, architecture)
    image = "#{@package}-#{os}"
    image << "-#{architecture}" if architecture
    image
  end

  def docker_run(os, architecture)
    id = os
    id = "#{id}-#{architecture}" if architecture
    image = docker_image(os, architecture)
    build_command_line = [
      "docker",
      "build",
      "--cache-from", image,
      "--tag", image,
    ]
    run_command_line = [
      "docker",
      "run",
      "--rm",
      "--tty",
      "--volume", "#{Dir.pwd}:/host:rw",
    ]
    if debug_build?
      build_command_line.concat(["--build-arg", "DEBUG=yes"])
      run_command_line.concat(["--env", "DEBUG=yes"])
    end
    if File.exist?(File.join(id, "Dockerfile"))
      docker_context = id
    else
      from = File.readlines(File.join(id, "from")).find do |line|
        /^[a-z]/i =~ line
      end
      build_command_line.concat(["--build-arg", "FROM=#{from.chomp}"])
      docker_context = os
    end
    build_command_line.concat(docker_build_options(os, architecture))
    run_command_line.concat(docker_run_options(os, architecture))
    build_command_line << docker_context
    run_command_line.concat([image, "/host/build.sh"])

    sh(*build_command_line)
    sh(*run_command_line)
  end

  def docker_build_options(os, architecture)
    []
  end

  def docker_run_options(os, architecture)
    []
  end

  def docker_pull(os, architecture)
    image = docker_image(os, architecture)
    command_line = [
      "docker",
      "pull",
      image,
    ]
    command_line.concat(docker_pull_options(os, architecture))
    sh(*command_line)
  end

  def docker_pull_options(os, architecture)
    []
  end

  def docker_push(os, architecture)
    image = docker_image(os, architecture)
    command_line = [
      "docker",
      "push",
      image,
    ]
    command_line.concat(docker_push_options(os, architecture))
    sh(*command_line)
  end

  def docker_push_options(os, architecture)
    []
  end

  def define_dist_task
    define_archive_task
    desc "Create release package"
    task :dist => [@archive_name]
  end

  def enable_apt?
    true
  end

  def apt_targets
    return [] unless enable_apt?

    targets = (ENV["APT_TARGETS"] || "").split(",")
    targets = apt_targets_default if targets.empty?

    targets.find_all do |target|
      Dir.exist?(File.join(apt_dir, target))
    end
  end

  def apt_targets_default
    # Disable arm64 targets by default for now
    # because they require some setups on host.
    [
      "debian-stretch",
      # "debian-stretch-arm64",
      "debian-buster",
      # "debian-stretch-arm64",
      "ubuntu-xenial",
      # "ubuntu-xenial-arm64",
      "ubuntu-bionic",
      # "ubuntu-bionic-arm64",
      "ubuntu-eoan",
      # "ubuntu-eoan-arm64",
      "ubuntu-focal",
      # "ubuntu-focal-arm64",
    ]
  end

  def deb_archive_base_name
    "#{@package}-#{@deb_archive_base_name_version}"
  end

  def deb_archive_name
    "#{@package}-#{@deb_upstream_version}.tar.gz"
  end

  def apt_dir
    "apt"
  end

  def apt_build
    tmp_dir = "#{apt_dir}/tmp"
    rm_rf(tmp_dir)
    mkdir_p(tmp_dir)
    cp(deb_archive_name,
       File.join(tmp_dir, deb_archive_name))
    Dir.glob("debian*") do |debian_dir|
      cp_r(debian_dir, "#{tmp_dir}/#{debian_dir}")
    end

    env_sh = "#{apt_dir}/env.sh"
    File.open(env_sh, "w") do |file|
      file.puts(<<-ENV)
PACKAGE=#{@package}
VERSION=#{@deb_upstream_version}
      ENV
    end

    apt_targets.each do |target|
      cd(apt_dir) do
        distribution, version, architecture = target.split("-", 3)
        os = "#{distribution}-#{version}"
        docker_run(os, architecture)
      end
    end
  end

  def define_apt_task
    namespace :apt do
      source_build_sh = "#{__dir__}/apt/build.sh"
      build_sh = "#{apt_dir}/build.sh"
      repositories_dir = "#{apt_dir}/repositories"

      file build_sh => source_build_sh do
        cp(source_build_sh, build_sh)
      end

      directory repositories_dir

      desc "Build deb packages"
      if enable_apt?
        build_dependencies = [
          deb_archive_name,
          build_sh,
          repositories_dir,
        ]
      else
        build_dependencies = []
      end
      task :build => build_dependencies do
        apt_build if enable_apt?
      end
    end

    desc "Release APT repositories"
    apt_tasks = [
      "apt:build",
    ]
    task :apt => apt_tasks
  end

  def enable_yum?
    true
  end

  def yum_targets
    return [] unless enable_yum?

    targets = (ENV["YUM_TARGETS"] || "").split(",")
    targets = yum_targets_default if targets.empty?

    targets.find_all do |target|
      Dir.exist?(File.join(yum_dir, target))
    end
  end

  def yum_targets_default
    # Disable aarch64 targets by default for now
    # because they require some setups on host.
    [
      "centos-6",
      "centos-7",
      # "centos-7-aarch64",
      "centos-8",
      # "centos-8-aarch64",
    ]
  end

  def rpm_archive_base_name
    "#{@package}-#{@rpm_version}"
  end

  def rpm_archive_name
    "#{rpm_archive_base_name}.tar.gz"
  end

  def yum_dir
    "yum"
  end

  def yum_build_sh
    "#{yum_dir}/build.sh"
  end

  def yum_expand_variable(key)
    case key
    when "PACKAGE"
      @rpm_package
    when "VERSION"
      @rpm_version
    when "RELEASE"
      @rpm_release
    else
      nil
    end
  end

  def yum_spec_in_path
    "#{yum_dir}/#{@rpm_package}.spec.in"
  end

  def yum_build
    tmp_dir = "#{yum_dir}/tmp"
    rm_rf(tmp_dir)
    mkdir_p(tmp_dir)
    cp(rpm_archive_name,
       File.join(tmp_dir, rpm_archive_name))

    env_sh = "#{yum_dir}/env.sh"
    File.open(env_sh, "w") do |file|
      file.puts(<<-ENV)
SOURCE_ARCHIVE=#{rpm_archive_name}
PACKAGE=#{@rpm_package}
VERSION=#{@rpm_version}
RELEASE=#{@rpm_release}
      ENV
    end

    spec = "#{tmp_dir}/#{@rpm_package}.spec"
    spec_in_data = File.read(yum_spec_in_path)
    spec_data = substitute_content(spec_in_data) do |key, matched|
      yum_expand_variable(key) || matched
    end
    File.open(spec, "w") do |spec_file|
      spec_file.print(spec_data)
    end

    yum_targets.each do |target|
      cd(yum_dir) do
        distribution, version, architecture = target.split("-", 3)
        os = "#{distribution}-#{version}"
        docker_run(os, architecture)
      end
    end
  end

  def define_yum_task
    namespace :yum do
      source_build_sh = "#{__dir__}/yum/build.sh"
      file yum_build_sh => source_build_sh do
        cp(source_build_sh, yum_build_sh)
      end

      repositories_dir = "#{yum_dir}/repositories"
      directory repositories_dir

      desc "Build RPM packages"
      if enable_yum?
        build_dependencies = [
          repositories_dir,
          rpm_archive_name,
          yum_build_sh,
          yum_spec_in_path,
        ]
      else
        build_dependencies = []
      end
      task :build => build_dependencies do
        yum_build if enable_yum?
      end
    end

    desc "Release Yum repositories"
    yum_tasks = [
      "yum:build",
    ]
    task :yum => yum_tasks
  end

  def define_version_task
    namespace :version do
      desc "Update versions"
      task :update do
        update_debian_changelog
        update_spec
      end
    end
  end

  def package_changelog_message
    "New upstream release."
  end

  def packager_name
    ENV["DEBFULLNAME"] || ENV["NAME"] || guess_packager_name_from_git
  end

  def guess_packager_name_from_git
    name = `git config --get user.name`.chomp
    return name unless name.empty?
    `git log -n 1 --format=%aN`.chomp
  end

  def packager_email
    ENV["DEBEMAIL"] || ENV["EMAIL"] || guess_packager_email_from_git
  end

  def guess_packager_email_from_git
    email = `git config --get user.email`.chomp
    return email unless email.empty?
    `git log -n 1 --format=%aE`.chomp
  end

  def update_content(path)
    if File.exist?(path)
      content = File.read(path)
    else
      content = ""
    end
    content = yield(content)
    File.open(path, "w") do |file|
      file.puts(content)
    end
  end

  def update_debian_changelog
    return unless enable_apt?

    Dir.glob("debian*") do |debian_dir|
      update_content("#{debian_dir}/changelog") do |content|
        <<-CHANGELOG.rstrip
#{@package} (#{@deb_upstream_version}-#{@deb_release}) unstable; urgency=low

  * New upstream release.

 -- #{packager_name} <#{packager_email}>  #{@release_time.rfc2822}

#{content}
        CHANGELOG
      end
    end
  end

  def update_spec
    return unless enable_yum?

    release_time = @release_time.strftime("%a %b %d %Y")
    update_content(yum_spec_in_path) do |content|
      content = content.sub(/^(%changelog\n)/, <<-CHANGELOG)
%changelog
* #{release_time} #{packager_name} <#{packager_email}> - #{@rpm_version}-#{@rpm_release}
- #{package_changelog_message}

      CHANGELOG
      content = content.sub(/^(Release:\s+)\d+/, "\\11")
      content.rstrip
    end
  end

  def define_docker_tasks
    namespace :docker do
      pull_tasks = []
      push_tasks = []

      (apt_targets + yum_targets).each do |target|
        distribution, version, architecture = target.split("-", 3)
        os = "#{distribution}-#{version}"

        namespace :pull do
          desc "Pull built image for #{target}"
          task target do
            docker_pull(os, architecture)
          end
          pull_tasks << "docker:pull:#{target}"
        end

        namespace :push do
          desc "Push built image for #{target}"
          task target do
            docker_push(os, architecture)
          end
          push_tasks << "docker:push:#{target}"
        end
      end

      desc "Pull built images"
      task :pull => pull_tasks

      desc "Push built images"
      task :push => push_tasks
    end
  end
end
