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
# $Revision: #3 $

import os
import json

import boto3
from botocore.exceptions import ClientError

import log

class CloudFormationClientWrapper(log.ClientWrapper):

    BACKOFF_BASE_SECONDS = 0.1
    BACKOFF_MAX_SECONDS = 20.0
    BACKOFF_MAX_TRYS = 5

    def __init__(self, wrapped_client):
        super(CloudFormationClientWrapper, self).__init__(wrapped_client)

    def __getattr__(self, attr):
        orig_attr = super(CloudFormationClientWrapper, self).__getattr__(attr)
        if callable(orig_attr):

            def __try_with_backoff(*args, **kwargs):
                # http://www.awsarchitectureblog.com/2015/03/backoff.html
                backoff = self.BACKOFF_BASE_SECONDS
                count = 1
                while True:
                    try:
                        return orig_attr(*args, **kwargs)
                    except (ClientError,EndpointConnectionError,IncompleteReadError,ConnectionError,BotoCoreError,UnknownEndpointError) as e:

                        if count == self.BACKOFF_MAX_TRYS or (hasattr(e, 'response') and e.response['Error']['Code'] != 'Throttling'):                    
                            raise e

                        backoff = min(self.BACKOFF_MAX_SECONDS, random.uniform(self.BACKOFF_BASE_SECONDS, backoff * 3.0))
                        if self.verbose:
                            self.log(attr, 'throttled attempt {}. Sleeping {} seconds'.format(count, backoff))
                        time.sleep(backoff)
                        count += 1

            return __try_with_backoff

        else:
            return orig_attr


lambda_name = os.environ.get('AWS_LAMBDA_FUNCTION_NAME')
current_region = os.environ.get('AWS_REGION')

class StackS3Configuration(object):
    stack_name = ""
    configuration_bucket = ""

def get_configuration_bucket():  
    configuration = StackS3Configuration()

    cloud_formation_client = CloudFormationClientWrapper(boto3.client('cloudformation', region_name=current_region))
 
    stack_definitions = cloud_formation_client.describe_stack_resources(PhysicalResourceId = lambda_name)
    for stack_definition in stack_definitions['StackResources']:
            if stack_definition.get('LogicalResourceId',None) == 'Configuration':
                configuration.configuration_bucket = stack_definition['PhysicalResourceId']
                configuration.stack_name = stack_definition['StackName']
                break
    return configuration    

# Stack ARN format: arn:aws:cloudformation:{region}:{account}:stack/{name}/{uuid}

def get_stack_name_from_stack_arn(arn):
    return arn.split('/')[1]


def get_region_from_stack_arn(arn):
    return arn.split(':')[3]


def get_account_id_from_stack_arn(arn):
    return arn.split(':')[4]

def get_cloud_canvas_metadata(resource, metadata_name):

    metadata_string = resource.get('Metadata', None)
    if metadata_string is None: return
    
    try:
        metadata = json.loads(metadata_string)
    except ValueError as e:
        raise ValidationError('Could not parse CloudCanvas {} Metadata: {}. {}'.format(metadata_name, metadata_string, e))

    cloud_canvas_metadata = metadata.get('CloudCanvas', None)
    if cloud_canvas_metadata is None: return

    return cloud_canvas_metadata.get(metadata_name, None)


RESOURCE_ARN_PATTERNS = {
    'AWS::DynamoDB::Table': 'arn:aws:dynamodb:{region}:{account_id}:table/{resource_name}',
    'AWS::Lambda::Function': 'arn:aws:lambda:{region}:{account_id}:function:{resource_name}',
    'AWS::SQS::Queue': 'arn:aws:sqs:{region}:{account_id}:{resource_name}',
    'AWS::SNS::Topic': 'arn:aws:sns:{region}:{account_id}:{resource_name}',
    'AWS::S3::Bucket': 'arn:aws:s3:::{resource_name}'
}


def get_resource_arn(stack_arn, resource_type, resource_name):
    
    pattern = RESOURCE_ARN_PATTERNS.get(resource_type, None)
    if pattern is None:
        raise ValidationError('Unsupported ARN mapping for resource type {} on resource {}. To add support for additional resource types, add an entry to RESOURCE_ARN_PATTERNS in project-code\discovery_utils.py.'.format(resource_type, resource_name))

    return pattern.format(
        region=get_region_from_stack_arn(stack_arn),
        account_id=get_account_id_from_stack_arn(stack_arn),
        resource_name=resource_name)

