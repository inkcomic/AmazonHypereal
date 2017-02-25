#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
# $Revision: #2 $

import dateutil
import json
import mock
import random
import unittest

import boto3
from botocore.exceptions import ClientError

import stack_info
import mock_aws
from errors import ValidationError

MOCK_CLIENT = 'test-client'

MOCK_REGION = 'test-region'
MOCK_ACCOUNT_ID = 'test-account-id'
MOCK_STACK_NAME = 'test-stack-name'
MOCK_STACK_UUID = 'test-stack-uuid'

def make_stack_arn(stack_name):
    return 'arn:aws:cloudformation:{region}:{account}:stack/{name}/{uuid}'.format(
        region = MOCK_REGION,
        account = MOCK_ACCOUNT_ID,
        name = stack_name,
        uuid = MOCK_STACK_UUID)

MOCK_STACK_ARN = make_stack_arn(MOCK_STACK_NAME)

MOCK_LOGICAL_RESOURCE_ID = 'test-logical-resource-id'
MOCK_PHYSICAL_RESOURCE_ID = 'test-physical-resource-id'
MOCK_RESOURCE_TYPE = 'test-resource-type'
MOCK_RESOURCE_STATUS = 'test-resource-status'
MOCK_RESOURCE_STATUS_REASON = 'test-resource-status-reason'
MOCK_RESOURCE_LAST_UPDATED_TIMESTAMP = '2011-06-21T20:15:58Z'

def make_resource_summary(
    physical_resource_id = MOCK_PHYSICAL_RESOURCE_ID, 
    logical_resource_id = MOCK_LOGICAL_RESOURCE_ID, 
    resource_type = MOCK_RESOURCE_TYPE, 
    resource_status = MOCK_RESOURCE_STATUS,
    resource_status_reason = MOCK_RESOURCE_STATUS_REASON,
    last_updated_timestamp = MOCK_RESOURCE_LAST_UPDATED_TIMESTAMP):
    return {
        'LogicalResourceId': logical_resource_id,
        'PhysicalResourceId': physical_resource_id,
        'ResourceType': resource_type,
        'ResourceStatus': resource_status,
        'ResourceStatusReason': resource_status_reason,
        'LastUpdatedTimestamp': last_updated_timestamp
    }

def make_random_resource_summary():
    id = str(random.randint(100000, 999999))
    return make_resource_summary(
        physical_resource_id = 'test-physical-resource-id-' + id,
        logical_resource_id = 'test-logical-resource-id-' + id,
        resource_type =  'test-resource-type-' + id
    )

MOCK_RESOURCE_SUMMARY = make_resource_summary()

MOCK_RESOURCE_SUMMARY_LIST = [ MOCK_RESOURCE_SUMMARY ]

MOCK_TEMPLATE = 'mock-template'

def make_list_stack_resources_response(resource_summary_list):
    return {
        'StackResourceSummaries': resource_summary_list
    }

def make_template_with_stack_type(stack_type):
    return {
        "Metadata": {
            "CloudCanvas": {
                "StackType": stack_type
            }
        }
    }

def make_get_template_response(template):
    return {
        'TemplateBody': json.dumps(template)
    }

def make_parameter(key, value):
    return {
        "ParameterKey": key,
        "ParameterValue": value
    }

MOCK_PARAMETER_NAME = "test-parameter-name"
MOCK_PARAMETER_VALUE = "test-parameter-value"

MOCK_PARAMETER = make_parameter(MOCK_PARAMETER_NAME, MOCK_PARAMETER_VALUE)

MOCK_PARAMETERS = [ MOCK_PARAMETER ]

MOCK_STACK_TYPE = 'test-stack-type'

def make_stack_description(stack_type):
    return {
        "Parameters": [
            make_parameter("CloudCanvasStack", stack_type)
        ]
    }

def make_describe_stacks_response(stack_description):
    return {
        "Stacks": [
            stack_description
        ]
    }

MOCK_STACK_DESCRIPTION = {
    "Parameters": MOCK_PARAMETERS
}

def make_random_parameter():
    id = str(random.randint(100000, 999999))
    return {
        'ParameterKey': 'test-parameter-name-' + id,
        'ParameterValue': 'test-parameter-value-' + id
    }

MOCK_RESOURCE_DESCRIPTION = 'test-description'

MOCK_RESOURCE_METADATA = {
    'Test': 'Metadata'
}

MOCK_RESOURCE_DETAIL = {
    'StackName': MOCK_STACK_NAME,
    'StackId': MOCK_STACK_ARN,
    'LogicalResourceId': MOCK_LOGICAL_RESOURCE_ID,
    'PhysicalResourceId': MOCK_PHYSICAL_RESOURCE_ID,
    'ResourceType': MOCK_RESOURCE_TYPE,
    'LastUpdatedTimestamp': MOCK_RESOURCE_LAST_UPDATED_TIMESTAMP,
    'ResourceStatus': MOCK_RESOURCE_STATUS,
    'ResourceStatusReason': MOCK_RESOURCE_STATUS_REASON,
    'Description': MOCK_RESOURCE_DESCRIPTION,
    'Metadata': json.dumps(MOCK_RESOURCE_METADATA)
}

MOCK_RESOURCE_DETAIL_RESPONSE = {
    'StackResourceDetail': MOCK_RESOURCE_DETAIL
}

class Test_stack_info_get_stack_info(unittest.TestCase):

    def test_with_project_stack(self):
        mock_response = make_describe_stacks_response(make_stack_description(stack_info.StackInfo.STACK_TYPE_PROJECT))
        with mock_aws.patch_client('cloudformation', 'describe_stacks', return_value = mock_response) as mock_describe_stacks:
            result = stack_info.get_stack_info(MOCK_STACK_ARN)
            self.assertIsInstance(result, stack_info.ProjectInfo)
            self.assertEqual(result.stack_type, stack_info.StackInfo.STACK_TYPE_PROJECT)
            mock_describe_stacks.client_factory.assert_called_once_with('cloudformation', region_name=MOCK_REGION)
            mock_describe_stacks.assert_called_once_with(StackName=MOCK_STACK_ARN)

    def test_with_deployment_stack(self):
        mock_response = make_describe_stacks_response(make_stack_description(stack_info.StackInfo.STACK_TYPE_DEPLOYMENT))
        with mock_aws.patch_client('cloudformation', 'describe_stacks', return_value = mock_response) as mock_describe_stacks:
            result = stack_info.get_stack_info(MOCK_STACK_ARN)
            self.assertIsInstance(result, stack_info.DeploymentInfo)
            self.assertEqual(result.stack_type, stack_info.StackInfo.STACK_TYPE_DEPLOYMENT)
            mock_describe_stacks.client_factory.assert_called_once_with('cloudformation', region_name=MOCK_REGION)
            mock_describe_stacks.assert_called_once_with(StackName=MOCK_STACK_ARN)

    def test_with_deployment_access_stack(self):
        mock_response = make_describe_stacks_response(make_stack_description(stack_info.StackInfo.STACK_TYPE_DEPLOYMENT_ACCESS))
        with mock_aws.patch_client('cloudformation', 'describe_stacks', return_value = mock_response) as mock_describe_stacks:
            result = stack_info.get_stack_info(MOCK_STACK_ARN)
            self.assertIsInstance(result, stack_info.DeploymentAccessInfo)
            self.assertEqual(result.stack_type, stack_info.StackInfo.STACK_TYPE_DEPLOYMENT_ACCESS)
            mock_describe_stacks.client_factory.assert_called_once_with('cloudformation', region_name=MOCK_REGION)
            mock_describe_stacks.assert_called_once_with(StackName=MOCK_STACK_ARN)

    def test_with_resource_group_stack(self):
        mock_response = make_describe_stacks_response(make_stack_description(stack_info.StackInfo.STACK_TYPE_RESOURCE_GROUP))
        with mock_aws.patch_client('cloudformation', 'describe_stacks', return_value = mock_response) as mock_describe_stacks:
            result = stack_info.get_stack_info(MOCK_STACK_ARN)
            self.assertIsInstance(result, stack_info.ResourceGroupInfo)
            self.assertEqual(result.stack_type, stack_info.StackInfo.STACK_TYPE_RESOURCE_GROUP)
            mock_describe_stacks.client_factory.assert_called_once_with('cloudformation', region_name=MOCK_REGION)
            mock_describe_stacks.assert_called_once_with(StackName=MOCK_STACK_ARN)

    def test_with_no_stack_type(self):
        mock_response = make_describe_stacks_response(MOCK_STACK_DESCRIPTION)
        with mock_aws.patch_client('cloudformation', 'describe_stacks', return_value = mock_response) as mock_describe_stacks:
            with self.assertRaisesRegexp(ValidationError, MOCK_STACK_ARN):
                stack_info.get_stack_info(MOCK_STACK_ARN)
            mock_describe_stacks.client_factory.assert_called_once_with('cloudformation', region_name=MOCK_REGION)
            mock_describe_stacks.assert_called_once_with(StackName=MOCK_STACK_ARN)

class Test_stack_info_ParametersDict(unittest.TestCase):

    def test_getitem(self):
        target = stack_info.ParametersDict(MOCK_STACK_ARN)
        target['key1'] = 'value1'
        target['key2'] = 'value2'
        self.assertEquals(target['key1'], 'value1')
        with self.assertRaisesRegexp(ValidationError, 'not-present'):
            target['not-present']
        with self.assertRaisesRegexp(ValidationError, MOCK_STACK_ARN):
            target['not-present']


class Test_stack_info_StackInfo(unittest.TestCase):

    def test_stack_arn(self):
        expected_stack_arn = 'stack-arn'
        target = stack_info.StackInfo(expected_stack_arn, MOCK_STACK_TYPE)
        actual_stack_arn = target.stack_arn
        self.assertEquals(actual_stack_arn, expected_stack_arn)

    def test_stack_type(self):
        expected_stack_type = 'stack-type'
        target = stack_info.StackInfo(MOCK_STACK_ARN, expected_stack_type)
        actual_stack_type = target.stack_type
        self.assertEquals(actual_stack_type, expected_stack_type)

    def test_client_created(self):
        with mock.patch('boto3.client') as mock_client_factory:
            target = stack_info.StackInfo(MOCK_STACK_ARN, MOCK_STACK_TYPE)
            actual_client = target.client
            actual_client_2 = target.client
            self.assertIsNotNone(actual_client)
            self.assertIs(actual_client, actual_client_2)
            mock_client_factory.assert_called_once_with('cloudformation', region_name=MOCK_REGION)

    def test_client_provided(self):
        expected_client = 'expected-client'
        target = stack_info.StackInfo(MOCK_STACK_ARN, MOCK_STACK_TYPE, client=expected_client)
        actual_client = target.client
        self.assertIs(actual_client, expected_client)

    def test_stack_name(self):
        expected_stack_name = MOCK_STACK_NAME
        target = stack_info.StackInfo(MOCK_STACK_ARN, MOCK_STACK_TYPE)
        actual_stack_name = target.stack_name
        self.assertEquals(actual_stack_name, expected_stack_name)

    def test_region(self):
        expected_region = MOCK_REGION
        target = stack_info.StackInfo(MOCK_STACK_ARN, MOCK_STACK_TYPE)
        actual_region = target.region
        self.assertEquals(actual_region, expected_region)

    def test_account_id(self):
        expected_account_id = MOCK_ACCOUNT_ID
        target = stack_info.StackInfo(MOCK_STACK_ARN, MOCK_STACK_TYPE)
        actual_account_id = target.account_id
        self.assertEquals(actual_account_id, expected_account_id)

    def test_resources(self):
        mock_resource_summary_1 = make_random_resource_summary()
        mock_resource_summary_2 = make_random_resource_summary()
        mock_resource_summary_list = [ mock_resource_summary_1, mock_resource_summary_2 ]
        mock_response = make_list_stack_resources_response(mock_resource_summary_list)
        mock_resources = [ 'mock-resource-info-1', 'mock-resource-info-2' ]
        with mock_aws.patch_client('cloudformation', 'list_stack_resources', return_value = mock_response) as mock_list_stack_resources:
            with mock.patch('stack_info.ResourceInfo', side_effect = mock_resources) as mock_ResourceInfo:
                target = stack_info.StackInfo(MOCK_STACK_ARN, MOCK_STACK_TYPE)
                actual_resources = target.resources
                actual_resources_2 = target.resources
                self.assertItemsEqual(actual_resources, mock_resources)
                self.assertIs(actual_resources, actual_resources_2)
                self.assertEquals(actual_resources.stack_arn, target.stack_arn)
                mock_list_stack_resources.assert_called_once_with(StackName = MOCK_STACK_ARN)
                mock_ResourceInfo.assert_any_call(MOCK_STACK_ARN, mock_resource_summary_1, target.client)
                mock_ResourceInfo.assert_any_call(MOCK_STACK_ARN, mock_resource_summary_2, target.client)

    def test_stack_description_provided(self):
        target = stack_info.StackInfo(MOCK_STACK_ARN, MOCK_STACK_TYPE, stack_description = MOCK_STACK_DESCRIPTION)
        actual_stack_description = target.stack_description
        self.assertEquals(actual_stack_description, MOCK_STACK_DESCRIPTION)

    def test_stack_description_loaded(self):
        mock_response = make_describe_stacks_response(MOCK_STACK_DESCRIPTION)
        with mock_aws.patch_client('cloudformation', 'describe_stacks', return_value = mock_response) as mock_describe_stacks:
            target = stack_info.StackInfo(MOCK_STACK_ARN, MOCK_STACK_TYPE)
            actual_stack_description = target.stack_description
            actual_stack_description_2 = target.stack_description
            self.assertEquals(actual_stack_description, MOCK_STACK_DESCRIPTION)
            self.assertIs(actual_stack_description, actual_stack_description_2)
            mock_describe_stacks.assert_called_once_with(StackName = MOCK_STACK_ARN)

    def test_parameters(self):
        key1 = 'key1'
        value1 = 'value1'
        key2 = 'key2'
        value2 = 'value2'
        mock_parameters = [ make_parameter(key1, value1), make_parameter(key2, value2) ]
        mock_stack_description = {
            "Parameters": mock_parameters
        }
        mock_parameters_dict = mock.MagicMock()
        with mock.patch('stack_info.StackInfo.stack_description', new_callable=mock.PropertyMock, return_value = mock_stack_description):
            with mock.patch('stack_info.ParametersDict', return_value = mock_parameters_dict) as mock_ParametersDict:
                target = stack_info.StackInfo(MOCK_STACK_ARN, MOCK_STACK_TYPE)
                actual_parameters = target.parameters
                actual_parameters_2 = target.parameters
                self.assertIs(actual_parameters, mock_parameters_dict)
                self.assertIs(actual_parameters_2, mock_parameters_dict)
                mock_ParametersDict.assert_called_once_with(MOCK_STACK_ARN)
                mock_parameters_dict.__setitem__.assert_any_call(key1, value1)
                mock_parameters_dict.__setitem__.assert_any_call(key2, value2)


class Test_stack_info_ResourceInfoList(unittest.TestCase):

    def test_constructor(self):
        target = stack_info.ResourceInfoList(MOCK_STACK_ARN)
        self.assertIs(target.stack_arn, MOCK_STACK_ARN)

    def test_find_resource_no_expected_type_not_optional_present(self):
        test_value = 'test-value'
        expected_resource = mock.MagicMock(test_attr = test_value)
        target = stack_info.ResourceInfoList(MOCK_STACK_ARN)
        target.append(mock.MagicMock(test_attr = 'unexpected-1'))
        target.append(expected_resource)
        target.append(mock.MagicMock(test_attr = 'unexpected-2'))
        actual_resource = target._ResourceInfoList__find_resource('test_attr', test_value, None, False)
        self.assertIs(actual_resource, expected_resource)

    def test_find_resource_no_expected_type_not_optional_not_present(self):
        test_value = 'test-value'
        target = stack_info.ResourceInfoList(MOCK_STACK_ARN)
        target.append(mock.MagicMock(test_attr = 'unexpected-1'))
        target.append(mock.MagicMock(test_attr = 'unexpected-2'))
        with self.assertRaisesRegexp(ValidationError, test_value):
            target._ResourceInfoList__find_resource('test_attr', test_value, None, False)

    def test_find_resource_no_expected_type_optional_not_present(self):
        test_value = 'test-value'
        target = stack_info.ResourceInfoList(MOCK_STACK_ARN)
        target.append(mock.MagicMock(test_attr = 'unexpected-1'))
        target.append(mock.MagicMock(test_attr = 'unexpected-2'))
        actual_resource = target._ResourceInfoList__find_resource('test_attr', test_value, None, True)
        self.assertIsNone(actual_resource)

    def test_find_resource_expected_type_not_optional_present(self):
        test_value = 'test-value'
        expected_type = 'test-type'
        expected_resource = mock.MagicMock(test_attr = test_value, type = expected_type)
        target = stack_info.ResourceInfoList(MOCK_STACK_ARN)
        target.append(mock.MagicMock(test_attr = 'unexpected-1'))
        target.append(expected_resource)
        target.append(mock.MagicMock(test_attr = 'unexpected-2'))
        actual_resource = target._ResourceInfoList__find_resource('test_attr', test_value, expected_type, False)
        self.assertIs(actual_resource, expected_resource)

    def test_find_resource_expected_type_not_optional_wrong_type(self):
        test_value = 'test-value'
        expected_type = 'test-type'
        expected_resource = mock.MagicMock(test_attr = test_value, type = 'unexpected-type')
        target = stack_info.ResourceInfoList(MOCK_STACK_ARN)
        target.append(mock.MagicMock(test_attr = 'unexpected-1'))
        target.append(expected_resource)
        target.append(mock.MagicMock(test_attr = 'unexpected-2'))
        with self.assertRaisesRegexp(ValidationError, expected_type):
            target._ResourceInfoList__find_resource('test_attr', test_value, expected_type, False)

    def test_get_by_logical_id_default(self):
        expected_resource = 'test-resource'
        logical_id = 'test-id'
        with mock.patch('stack_info.ResourceInfoList._ResourceInfoList__find_resource', return_value = expected_resource) as mock_find_resource:
            target = stack_info.ResourceInfoList(MOCK_STACK_ARN)
            actual_resource = target.get_by_logical_id(logical_id)
            self.assertIs(actual_resource, expected_resource)
            mock_find_resource.assert_called_once_with('logical_id', logical_id, None, False)

    def test_get_by_logical_id_with_args(self):
        expected_resource = 'test-resource'
        logical_id = 'test-id'
        expected_type = 'test-type'
        optional = True
        with mock.patch('stack_info.ResourceInfoList._ResourceInfoList__find_resource', return_value = expected_resource) as mock_find_resource:
            target = stack_info.ResourceInfoList(MOCK_STACK_ARN)
            actual_resource = target.get_by_logical_id(logical_id, expected_type = expected_type, optional = optional)
            self.assertIs(actual_resource, expected_resource)
            mock_find_resource.assert_called_once_with('logical_id', logical_id, expected_type, optional)

    def test_get_by_physical_id_default(self):
        expected_resource = 'test-resource'
        physical_id = 'test-id'
        with mock.patch('stack_info.ResourceInfoList._ResourceInfoList__find_resource', return_value = expected_resource) as mock_find_resource:
            target = stack_info.ResourceInfoList(MOCK_STACK_ARN)
            actual_resource = target.get_by_physical_id(physical_id)
            self.assertIs(actual_resource, expected_resource)
            mock_find_resource.assert_called_once_with('physical_id', physical_id, None, False)

    def test_get_by_physical_id_with_args(self):
        expected_resource = 'test-resource'
        physical_id = 'test-id'
        expected_type = 'test-type'
        optional = True
        with mock.patch('stack_info.ResourceInfoList._ResourceInfoList__find_resource', return_value = expected_resource) as mock_find_resource:
            target = stack_info.ResourceInfoList(MOCK_STACK_ARN)
            actual_resource = target.get_by_physical_id(physical_id, expected_type = expected_type, optional = optional)
            self.assertIs(actual_resource, expected_resource)
            mock_find_resource.assert_called_once_with('physical_id', physical_id, expected_type, optional)

    def test_get_by_type(self):
        test_value = 'test-value'
        expected_type = 'test-type'
        expected_resource = mock.MagicMock(test_attr = test_value, type = expected_type)
        expected_list = [ expected_resource ]
        target = stack_info.ResourceInfoList(MOCK_STACK_ARN)
        target.append(mock.MagicMock(test_attr = 'unexpected-1', type = 'unexpected-type' ))
        target.append(expected_resource)
        target.append(mock.MagicMock(test_attr = 'unexpected-2', type = 'unexpected-type'))
        actual_list = target.get_by_type(expected_type)
        self.assertItemsEqual(actual_list, expected_list)

class Test_stack_info_ResourceInfo(unittest.TestCase):

    def test_constructor(self):
        target = stack_info.ResourceInfo(MOCK_STACK_ARN, MOCK_RESOURCE_SUMMARY, MOCK_CLIENT)
        self.assertIs(target.stack_arn, MOCK_STACK_ARN)
        self.assertIs(target.client, MOCK_CLIENT)
        self.assertIs(target.physical_id, MOCK_PHYSICAL_RESOURCE_ID)
        self.assertIs(target.logical_id, MOCK_LOGICAL_RESOURCE_ID)
        self.assertIs(target.type, MOCK_RESOURCE_TYPE)
        self.assertIs(target.status, MOCK_RESOURCE_STATUS)
        self.assertIs(target.status_reason, MOCK_RESOURCE_STATUS_REASON)
        self.assertEquals(target.last_updated_time, dateutil.parser.parse(MOCK_RESOURCE_LAST_UPDATED_TIMESTAMP))

    def test_description(self):
        client = mock.MagicMock()
        client.describe_stack_resource.return_value = MOCK_RESOURCE_DETAIL_RESPONSE
        target = stack_info.ResourceInfo(MOCK_STACK_ARN, MOCK_RESOURCE_SUMMARY, client)
        actual_description_1 = target.description
        actual_description_2 = target.description
        self.assertIs(actual_description_1, MOCK_RESOURCE_DESCRIPTION)
        self.assertIs(actual_description_2, MOCK_RESOURCE_DESCRIPTION)
        client.describe_stack_resource.assert_called_once_with(StackName=MOCK_STACK_ARN, LogicalResourceId=MOCK_LOGICAL_RESOURCE_ID)

    def test_metadata(self):
        client = mock.MagicMock()
        client.describe_stack_resource.return_value = MOCK_RESOURCE_DETAIL_RESPONSE
        target = stack_info.ResourceInfo(MOCK_STACK_ARN, MOCK_RESOURCE_SUMMARY, client)
        actual_metadata_1 = target.metadata
        actual_metadata_2 = target.metadata
        self.assertEquals(actual_metadata_1, MOCK_RESOURCE_METADATA)
        self.assertEquals(actual_metadata_2, MOCK_RESOURCE_METADATA)
        client.describe_stack_resource.assert_called_once_with(StackName=MOCK_STACK_ARN, LogicalResourceId=MOCK_LOGICAL_RESOURCE_ID)

    def test_get_cloud_canvas_metadata_found(self):
        target = stack_info.ResourceInfo(MOCK_STACK_ARN, MOCK_RESOURCE_SUMMARY, MOCK_CLIENT)
        expected_value = 'value'
        mock_metadata = {
            'CloudCanvas': {
                'a': {
                    'b':{
                        'c': expected_value
                    }
                }
            }
        }
        with mock.patch('stack_info.ResourceInfo.metadata', new_callable=mock.PropertyMock, return_value = mock_metadata):
            actual_value = target.get_cloud_canvas_metadata('a', 'b', 'c')
            self.assertEquals(actual_value, expected_value)


    def test_get_cloud_canvas_metadata_not_found(self):
        target = stack_info.ResourceInfo(MOCK_STACK_ARN, MOCK_RESOURCE_SUMMARY, MOCK_CLIENT)
        unexpected_value = 'value'
        mock_metadata = {
            'CloudCanvas': {
                'a': {
                    'b':{
                        'c': unexpected_value
                    }
                }
            }
        }
        with mock.patch('stack_info.ResourceInfo.metadata', new_callable=mock.PropertyMock, return_value = mock_metadata):
            actual_value = target.get_cloud_canvas_metadata('a', 'x', 'c')
            self.assertIsNone(actual_value)


class Test_stack_info_ProjectInfo(unittest.TestCase):

    def test_constructor(self):
        target = stack_info.ProjectInfo(MOCK_STACK_ARN, client = MOCK_CLIENT, stack_description = MOCK_STACK_DESCRIPTION)
        self.assertEquals(target.stack_type, stack_info.StackInfo.STACK_TYPE_PROJECT)
        self.assertIs(target.client, MOCK_CLIENT)
        self.assertIs(target.stack_description, MOCK_STACK_DESCRIPTION)

    def test_project_name(self):
        target = stack_info.ProjectInfo(MOCK_STACK_ARN)
        self.assertEquals(target.project_name, MOCK_STACK_NAME)

    def test_deployments(self):
        mock_deployment_1_name = 'dep1'
        mock_deployment_2_name = 'dep2'
        mock_deployment_1_stack_arn = make_stack_arn(mock_deployment_1_name)
        mock_deployment_2_stack_arn = make_stack_arn(mock_deployment_2_name)
        mock_deployment_1_access_stack_arn = make_stack_arn(mock_deployment_1_name + 'access')
        mock_deployment_2_access_stack_arn = make_stack_arn(mock_deployment_2_name + 'access')
        mock_project_settings = {
            'deployments': {
                mock_deployment_1_name: {
                    'DeploymentStackId': mock_deployment_1_stack_arn,
                    'DeploymentAccessStackId': mock_deployment_1_access_stack_arn
                },
                mock_deployment_2_name: {
                    'DeploymentStackId': mock_deployment_2_stack_arn,
                    'DeploymentAccessStackId': mock_deployment_2_access_stack_arn
                },                    
                '*': {}
            }
        }
        mock_deployments = [ 'test-deployment-info-1', 'test-deployment-info-2' ]
        with mock.patch('stack_info.DeploymentInfo', side_effect = mock_deployments) as mock_DeploymentInfo:
            with mock.patch('stack_info.ProjectInfo.project_settings', new_callable=mock.PropertyMock, return_value = mock_project_settings):
                target = stack_info.ProjectInfo(MOCK_STACK_ARN, client=MOCK_CLIENT)
                actual_deployments = target.deployments
                mock_DeploymentInfo.assert_any_call(mock_deployment_1_stack_arn, deployment_access_stack_arn = mock_deployment_1_access_stack_arn, client=MOCK_CLIENT, project_info = target)
                mock_DeploymentInfo.assert_any_call(mock_deployment_2_stack_arn, deployment_access_stack_arn = mock_deployment_2_access_stack_arn, client=MOCK_CLIENT, project_info = target)
                self.assertItemsEqual(actual_deployments, mock_deployments)

    def test_configuration_bucket(self):
        expected_configuration_bucket_id = 'test-id'
        mock_resource = mock.MagicMock(physical_id = expected_configuration_bucket_id)
        mock_resources = mock.MagicMock()
        mock_resources.get_by_logical_id.return_value = mock_resource
        with mock.patch('stack_info.ProjectInfo.resources', new = mock.PropertyMock( return_value = mock_resources )):
            target = stack_info.ProjectInfo(MOCK_STACK_ARN)
            actual_configuration_bucket_id = target.configuration_bucket
            self.assertEquals(actual_configuration_bucket_id, expected_configuration_bucket_id)
            mock_resources.get_by_logical_id.assert_called_once_with('Configuration', expected_type='AWS::S3::Bucket')

    def test_project_settings(self):

        mock_project_settings = {
            'Test': 'Setting'
        }

        mock_configuration_bucket_arn = 'mock-arn'

        mock_response = mock_aws.s3_get_object_response(json.dumps(mock_project_settings))

        with mock_aws.patch_client('s3', 'get_object', return_value = mock_response, reload = stack_info) as mock_get_object:
            with mock.patch('stack_info.ProjectInfo.configuration_bucket', new_callable=mock.PropertyMock, return_value = mock_configuration_bucket_arn):
                target = stack_info.ProjectInfo(MOCK_STACK_ARN)
                actual_project_settings = target.project_settings
                self.assertEquals(actual_project_settings, mock_project_settings)
                mock_get_object.assert_called_once_with(Bucket=mock_configuration_bucket_arn, Key='project-settings.json')

        
class Test_stack_info_DeploymentInfo(unittest.TestCase):

    def test_constructor(self):
        target = stack_info.DeploymentInfo(MOCK_STACK_ARN, client = MOCK_CLIENT, stack_description = MOCK_STACK_DESCRIPTION)
        self.assertEquals(target.stack_type, stack_info.StackInfo.STACK_TYPE_DEPLOYMENT)
        self.assertIs(target.client, MOCK_CLIENT)
        self.assertIs(target.stack_description, MOCK_STACK_DESCRIPTION)

    def test_deployment_name(self):
        mock_deployment_name = 'test-deployment'
        mock_parameters = { 'DeploymentName' : mock_deployment_name }
        with mock.patch('stack_info.DeploymentInfo.parameters', new = mock.PropertyMock( return_value = mock_parameters )):
            target = stack_info.DeploymentInfo(MOCK_STACK_ARN)
            self.assertEquals(target.deployment_name, mock_deployment_name)

    def test_deployment_access_provided(self):
        mock_deployment_access = 'test-deployment-access'
        target = stack_info.DeploymentInfo(MOCK_STACK_ARN, deployment_access_info = mock_deployment_access, client=MOCK_CLIENT)
        actual_deployment_access = target.deployment_access
        self.assertIs(actual_deployment_access, mock_deployment_access)

    def test_deployment_access_arn_provided(self):
        mock_deployment_access = 'test-deployment-access'
        with mock.patch('stack_info.DeploymentAccessInfo', return_value = mock_deployment_access) as mock_DeploymentAccessInfo:
            mock_deployment_access_stack_arn = 'deployment-access-stack-arn'
            target = stack_info.DeploymentInfo(MOCK_STACK_ARN, deployment_access_stack_arn = mock_deployment_access_stack_arn, client=MOCK_CLIENT)
            actual_deployment_access = target.deployment_access
            self.assertIs(actual_deployment_access, mock_deployment_access)
            mock_DeploymentAccessInfo.assert_called_once_with(mock_deployment_access_stack_arn, deployment_info = target, client=MOCK_CLIENT)

    def test_deployment_access_arn_discovered(self):
        mock_deployment_access_stack_arn = 'deployment-access-stack-arn'
        mock_describe_stacks_result = {
            'Stacks': [
                {
                    'StackId': mock_deployment_access_stack_arn,
                    'StackStatus': 'UPDATE_COMPLETE'
                }
            ]
        }
        mock_deployment_access_stack_name = MOCK_STACK_NAME + '-Access'
        mock_deployment_access = 'test-deployment-access'
        with mock.patch('stack_info.DeploymentAccessInfo', return_value = mock_deployment_access) as mock_DeploymentAccessInfo:
            with mock_aws.patch_client('cloudformation', 'describe_stacks', return_value = mock_describe_stacks_result) as mock_describe_stacks:
                target = stack_info.DeploymentInfo(MOCK_STACK_ARN)
                actual_deployment_access = target.deployment_access
                self.assertIs(actual_deployment_access, mock_deployment_access)
                mock_DeploymentAccessInfo.assert_called_once_with(mock_deployment_access_stack_arn, deployment_info = target, client=target.client)
                mock_describe_stacks.assert_called_once_with(StackName=mock_deployment_access_stack_name)

    def test_deployment_access_arn_discovered_with_access_denied(self):
        mock_describe_stacks_result = ClientError(
            {
                'Error': {
                    'Code': 'ValidationError'
                }
            },
            'describe-stacks'
        )
        mock_deployment_access_stack_name = MOCK_STACK_NAME + '-Access'
        with mock_aws.patch_client('cloudformation', 'describe_stacks', side_effect = mock_describe_stacks_result) as mock_describe_stacks:
            target = stack_info.DeploymentInfo(MOCK_STACK_ARN)
            actual_deployment_access = target.deployment_access
            self.assertIsNone(actual_deployment_access)
            mock_describe_stacks.assert_called_once_with(StackName=mock_deployment_access_stack_name)

    def test_deployment_access_arn_discovered_with_not_access_denied(self):
        mock_error_code = 'SomeErrorCode'
        mock_describe_stacks_result = ClientError(
            {
                'Error': {
                    'Code': mock_error_code
                }
            },
            'describe-stacks'
        )
        mock_deployment_access_stack_name = MOCK_STACK_NAME + '-Access'
        with mock_aws.patch_client('cloudformation','describe_stacks', side_effect = mock_describe_stacks_result) as mock_describe_stacks:
            target = stack_info.DeploymentInfo(MOCK_STACK_ARN)
            with self.assertRaisesRegexp(ClientError, mock_error_code):
                actual_deployment_access = target.deployment_access
            mock_describe_stacks.assert_called_once_with(StackName=mock_deployment_access_stack_name)

    def test_deployment_access_arn_discovered_with_deleted(self):
        mock_deployment_access_stack_arn = 'deployment-access-stack-arn'
        mock_describe_stacks_result = {
            'Stacks': [
                {
                    'StackId': 'wrong-stack-id',
                    'StackStatus': 'DELETE_COMPLETE'
                },
                {
                    'StackId': mock_deployment_access_stack_arn,
                    'StackStatus': 'UPDATE_COMPLETE'
                }
            ]
        }
        mock_deployment_access_stack_name = MOCK_STACK_NAME + '-Access'
        mock_deployment_access = 'test-deployment-access'
        with mock.patch('stack_info.DeploymentAccessInfo', return_value = mock_deployment_access) as mock_DeploymentAccessInfo:
            with mock_aws.patch_client('cloudformation', 'describe_stacks', return_value = mock_describe_stacks_result) as mock_describe_stacks:
                target = stack_info.DeploymentInfo(MOCK_STACK_ARN)
                actual_deployment_access = target.deployment_access
                self.assertIs(actual_deployment_access, mock_deployment_access)
                mock_DeploymentAccessInfo.assert_called_once_with(mock_deployment_access_stack_arn, deployment_info = target, client=target.client)
                mock_describe_stacks.assert_called_once_with(StackName=mock_deployment_access_stack_name)

    def test_deployment_access_arn_discovered_with_deleted_only(self):
        mock_deployment_access_stack_arn = 'deployment-access-stack-arn'
        mock_describe_stacks_result = {
            'Stacks': [
                {
                    'StackId': 'wrong-stack-id',
                    'StackStatus': 'DELETE_COMPLETE'
                },
                {
                    'StackId': mock_deployment_access_stack_arn,
                    'StackStatus': 'DELETE_COMPLETE'
                }
            ]
        }
        mock_deployment_access_stack_name = MOCK_STACK_NAME + '-Access'
        with mock_aws.patch_client('cloudformation', 'describe_stacks', return_value = mock_describe_stacks_result) as mock_describe_stacks:
            target = stack_info.DeploymentInfo(MOCK_STACK_ARN)
            actual_deployment_access = target.deployment_access
            self.assertIsNone(actual_deployment_access)
            mock_describe_stacks.assert_called_once_with(StackName=mock_deployment_access_stack_name)

    def test_project_provided(self):
        mock_project_info = 'test-project-info'
        target = stack_info.DeploymentInfo(MOCK_STACK_ARN, project_info=mock_project_info)
        self.assertIs(target.project, mock_project_info)

    def test_project_discovered(self):
        mock_project_stack_id = 'test-project-stack-id'
        mock_project = 'test-project-info'
        mock_parameters = { 'ProjectStackId' : mock_project_stack_id }
        with mock.patch('stack_info.ProjectInfo', return_value = mock_project) as mock_ProjectInfo:
            with mock.patch('stack_info.DeploymentInfo.parameters', new = mock.PropertyMock( return_value = mock_parameters )):
                target = stack_info.DeploymentInfo(MOCK_STACK_ARN, client=MOCK_CLIENT)
                actual_project = target.project
                actual_project_2 = target.project
                self.assertIs(actual_project, mock_project)
                self.assertIs(actual_project_2, mock_project)
                mock_ProjectInfo.assert_called_once_with(mock_project_stack_id, client=MOCK_CLIENT)

    def test_resource_groups(self):
        mock_physical_resource_id_1 = 'mock_physical_resource_id_1'
        mock_physical_resource_id_2 = 'mock_physical_resource_id_2'
        mock_logical_resource_id_1 = 'mock_logical_id_1'
        mock_logical_resource_id_2 = 'mock_logical_id_2'
        mock_resource_1 = mock.MagicMock()
        mock_resource_1.physical_id = mock_physical_resource_id_1
        mock_resource_1.logical_id = mock_logical_resource_id_1
        mock_resource_1.type = 'AWS::CloudFormation::Stack'
        mock_resource_2 = mock.MagicMock()
        mock_resource_2.physical_id = mock_physical_resource_id_2
        mock_resource_2.logical_id = mock_logical_resource_id_2
        mock_resource_2.type = 'AWS::CloudFormation::Stack'
        mock_resource_3 = mock.MagicMock()
        mock_resource_3.physical_id = 'not-used'
        mock_resource_3.logical_id = 'not-used'
        mock_resource_3.type = 'not-a-stack'
        mock_resource_4 = mock.MagicMock()
        mock_resource_4.physical_id = 'not-used'
        mock_resource_4.logical_id = 'not-used'
        mock_resource_4.type = 'not-a-stack'
        mock_resources = [ 
            mock_resource_3,
            mock_resource_1,
            mock_resource_4,
            mock_resource_2
        ]
        mock_resource_group_1 = 'test-resource-group-1'
        mock_resource_group_2 = 'test-resource-group-2'
        mock_resource_groups = [ mock_resource_group_1, mock_resource_group_2 ]
        with mock.patch('stack_info.ResourceGroupInfo', side_effect = mock_resource_groups) as mock_ResourceGroupInfo:
            with mock.patch('stack_info.DeploymentInfo.resources', new_callable=mock.PropertyMock, return_value=mock_resources):
                target = stack_info.DeploymentInfo(MOCK_STACK_ARN, client=MOCK_CLIENT)
                actual_resource_groups = target.resource_groups
                self.assertItemsEqual(actual_resource_groups, mock_resource_groups)
                mock_ResourceGroupInfo.assert_any_call(mock_physical_resource_id_1, resource_group_name=mock_logical_resource_id_1, client=target.client, deployment_info=target)
                mock_ResourceGroupInfo.assert_any_call(mock_physical_resource_id_2, resource_group_name=mock_logical_resource_id_2, client=target.client, deployment_info=target)


class Test_stack_info_DeploymentAccessInfo(unittest.TestCase):

    def test_constructor(self):
        target = stack_info.DeploymentAccessInfo(MOCK_STACK_ARN, client = MOCK_CLIENT, stack_description = MOCK_STACK_DESCRIPTION)
        self.assertEquals(target.stack_type, stack_info.StackInfo.STACK_TYPE_DEPLOYMENT_ACCESS)
        self.assertIs(target.client, MOCK_CLIENT)
        self.assertIs(target.stack_description, MOCK_STACK_DESCRIPTION)

    def test_deployment_provided(self):
        mock_deployment = 'test-deployment-stack-info'
        target = stack_info.DeploymentAccessInfo(MOCK_STACK_ARN, deployment_info = mock_deployment)
        self.assertEquals(target.deployment, mock_deployment)

    def test_deployment_discovered(self):
        mock_deployment_stack_arn = 'test-deployment-stack-arn'
        mock_client = 'mock_client'
        mock_deployment = 'test-deployment-info'
        mock_parameters = { 'DeploymentStackArn': mock_deployment_stack_arn }
        with mock.patch('stack_info.DeploymentInfo', return_value=mock_deployment) as mock_DeploymentInfo:
            with mock.patch('stack_info.DeploymentAccessInfo.parameters', new = mock.PropertyMock( return_value = mock_parameters )):
                target = stack_info.DeploymentAccessInfo(MOCK_STACK_ARN, client = mock_client)
                actual_deployment = target.deployment
                actual_deployment_2 = target.deployment
                self.assertIs(actual_deployment, mock_deployment)
                self.assertIs(actual_deployment_2, mock_deployment)
                mock_DeploymentInfo.assert_called_once_with(mock_deployment_stack_arn, deployment_access_info = target, client = mock_client)



class Test_stack_info_ResourceGroupInfo(unittest.TestCase):

    def test_constructor(self):
        target = stack_info.ResourceGroupInfo(MOCK_STACK_ARN, client = MOCK_CLIENT, stack_description = MOCK_STACK_DESCRIPTION)
        self.assertEquals(target.stack_type, stack_info.StackInfo.STACK_TYPE_RESOURCE_GROUP)
        self.assertIs(target.client, MOCK_CLIENT)
        self.assertIs(target.stack_description, MOCK_STACK_DESCRIPTION)

    def test_resource_group_name_provided(self):
        mock_resource_group_name = 'test_resource_group_name'
        target = stack_info.ResourceGroupInfo(MOCK_STACK_ARN, resource_group_name = mock_resource_group_name)
        self.assertEquals(target.resource_group_name, mock_resource_group_name)

    def test_resource_group_name_discovered(self):
        mock_resource_group_name = 'test-name'
        mock_parameters = { 'ResourceGroupName': mock_resource_group_name }
        with mock.patch('stack_info.ResourceGroupInfo.parameters', new = mock.PropertyMock( return_value = mock_parameters )):
            target = stack_info.ResourceGroupInfo(MOCK_STACK_ARN)
            actual_resource_group_name = target.resource_group_name
            self.assertEquals(actual_resource_group_name, mock_resource_group_name)

    def test_deployment_provided(self):
        mock_deployment = 'test_deployment'
        target = stack_info.ResourceGroupInfo(MOCK_STACK_ARN, deployment_info = mock_deployment)
        self.assertEquals(target.deployment, mock_deployment)

    def test_deployment_discovered(self):
        mock_deployment_stack_id = 'test-deployment-stack-id'
        mock_parameters = { 'DeploymentStackArn': mock_deployment_stack_id }
        mock_deployment = 'test-deployment'
        with mock.patch('stack_info.DeploymentInfo', return_value = mock_deployment) as mock_DeploymentInfo:
            with mock.patch('stack_info.ResourceGroupInfo.parameters', new = mock.PropertyMock( return_value = mock_parameters )):
                target = stack_info.ResourceGroupInfo(MOCK_STACK_ARN)
                actual_deployment = target.deployment
                actual_deployment_2 = target.deployment
                self.assertEquals(actual_deployment, mock_deployment)
                self.assertEquals(actual_deployment_2, mock_deployment)
                mock_DeploymentInfo.assert_called_once_with(mock_deployment_stack_id, client=target.client)


if __name__ == '__main__':
    unittest.main()
