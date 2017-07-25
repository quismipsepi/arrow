#!/usr/bin/env python

#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Utility for generating changelogs for fix versions
# requirements: pip install jira
# Set $JIRA_USERNAME, $JIRA_PASSWORD environment variables

from collections import defaultdict
from io import StringIO
import os
import sys

import jira.client

# ASF JIRA username
JIRA_USERNAME = os.environ.get("JIRA_USERNAME")
# ASF JIRA password
JIRA_PASSWORD = os.environ.get("JIRA_PASSWORD")

JIRA_API_BASE = "https://issues.apache.org/jira"

asf_jira = jira.client.JIRA({'server': JIRA_API_BASE},
                            basic_auth=(JIRA_USERNAME, JIRA_PASSWORD))


def get_issues_for_version(version):
    jql = ("project=ARROW "
           "AND fixVersion='{0}' "
           "AND status = Resolved "
           "AND resolution in (Fixed, Done) "
           "ORDER BY issuetype DESC").format(version)

    return asf_jira.search_issues(jql, maxResults=9999)


LINK_TEMPLATE = '[{0}](https://issues.apache.org/jira/browse/{0})'


def format_changelog_markdown(issues, out):
    issues_by_type = defaultdict(list)
    for issue in issues:
        issues_by_type[issue.fields.issuetype.name].append(issue)

    for issue_type, issue_group in sorted(issues_by_type.items()):
        issue_group.sort(key=lambda x: x.key)

        out.write('## {0}\n\n'.format(issue_type))
        for issue in issue_group:
            out.write('* {0} - {1}\n'.format(issue.key,
                                             issue.fields.summary))
        out.write('\n')


def format_changelog_website(issues, out):
    NEW_FEATURE = 'New Features and Improvements'
    BUGFIX = 'Bug Fixes'

    CATEGORIES = {
        'New Feature': NEW_FEATURE,
        'Improvement': NEW_FEATURE,
        'Wish': NEW_FEATURE,
        'Task': NEW_FEATURE,
        'Test': NEW_FEATURE,
        'Bug': BUGFIX
    }

    issues_by_category = defaultdict(list)
    for issue in issues:
        issue_type = issue.fields.issuetype.name
        website_category = CATEGORIES[issue_type]
        issues_by_category[website_category].append(issue)

    WEBSITE_ORDER = [NEW_FEATURE, BUGFIX]

    for issue_category in WEBSITE_ORDER:
        issue_group = issues_by_category[issue_category]
        issue_group.sort(key=lambda x: x.key)

        out.write('## {0}\n\n'.format(issue_category))
        for issue in issue_group:
            name = LINK_TEMPLATE.format(issue.key)
            out.write('* {0} - {1}\n'.format(name, issue.fields.summary))
        out.write('\n')


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('Usage: make_changelog.py $FIX_VERSION [$IS_WEBSITE]')

    buf = StringIO()

    for_website = len(sys.argv) > 2 and sys.argv[2] == '1'

    issues_for_version = get_issues_for_version(sys.argv[1])

    if for_website:
        format_changelog_website(issues_for_version, buf)
    else:
        format_changelog_markdown(issues_for_version, buf)

    print(buf.getvalue())
