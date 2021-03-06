#pragma once

#include "GeneretedFiles/TextualSearchService.grpc.pb.h"
#include "Service.h"

class ITextualSearchService;

class TextualSearchServiceImpl : public ::TextualSearchService::TextualSearch::Service, public Service
{
public:
	TextualSearchServiceImpl(ITextualSearchService& application): m_application(application){}
	virtual ~TextualSearchServiceImpl(){}
    ::grpc::Service* GetService(){
		return this;
	}
    ::grpc::Status Init(::grpc::ServerContext* context, const ::TextualSearchService::Params* request, ::TextualSearchService::Empty* response) override;
    ::grpc::Status IndexDocument(::grpc::ServerContext* context, const ::TextualSearchService::Empty* request, ::TextualSearchService::Empty* response) override;
	//::grpc::Status GetTopKDocuments(::grpc::ServerContext* context, const ::TextualSearchService::TopKDocumentsPerWordRequest* request, ::TextualSearchService::TopKDocumentsReply* response) override;
   	::grpc::Status Terminate(::grpc::ServerContext* context, const ::TextualSearchService::Empty* request, ::TextualSearchService::Empty* response) override;

private:
	ITextualSearchService& m_application;
};
