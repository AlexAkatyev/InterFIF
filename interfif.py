import datetime
from datetime import timedelta

import openpyxl
import requests.api
import time
import json.encoder
import sys


def getTypeSIFromStringCell(v):
    r = v[-12:]
    result = ''
    for s in r:
        if s in ['-', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9']:
            result += s
    return result


def getSerialNumberFromStringCell(v):
    r = v.split(',')[-1:][0]
    result = ''
    for s in r:
        if s not in [')', '(', '"']:
            result += s
    return result


# "verification_date": "2020-12-01T00:00:00Z",
def getStrFromDataCell(v):
    r = v.split(',')[1]
    r = r.split('.')[0]
    dd = ''
    for s in r:
        if s not in [')', '(', '"']:
            dd += s
    d = datetime.date(1900, 1, 1)
    d = d + timedelta(int(dd))
    result = d.strftime("%Y-%m-%dT00:00:00Z")
    return result


def getPost(cletB, cletD, cletF):
    u = 'https://fgis.gost.ru/fundmetrology/eapi/vri'
    param = {'year': cletF[:4], 'search': cletB + ' ' + cletD}
    time.sleep(1)
    resp = requests.get(url= u, params= param)
    # print(resp.text)
    data =  json.loads(resp.text)
    res = data['result']
    if res['count'] == 0:
        return ''
    for item in res['items']:
        if 'ДЮЮ' in item['result_docnum']:
            return item['result_docnum']
    return resp.text


def getShortArshinNumber(fullNumber):
    t = fullNumber.split('/')
    return t[-1]


# fileName = 'C://1//шаблон-1-2.xlsx'
fileName = sys.argv[1]
wb = openpyxl.load_workbook(fileName)
sheet = wb['Лист1']
cB = 2
cD = 4
cF = 6
cAI = 35
r = 2
while r < 65536:
    cletB = sheet.cell(row=r, column=cB).value
    if cletB == None:
        break
    vCellB = getTypeSIFromStringCell(cletB)
    cletD = sheet.cell(row=r, column=cD).value
    vCellD = getSerialNumberFromStringCell(cletD)
    cletF = sheet.cell(row=r, column=cF).value
    vCellF = getStrFromDataCell(cletF)
    sheet.cell(row=r, column=cAI).value = getShortArshinNumber(getPost(vCellB, vCellD, vCellF))
    r += 1
wb.save(fileName)



